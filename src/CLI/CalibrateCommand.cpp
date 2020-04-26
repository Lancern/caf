#include "Command.h"
#include "Diagnostics.h"
#include "RegisterCommand.h"

#include "Infrastructure/Memory.h"
#include "Infrastructure/Stream.h"
#include "Basic/CAFStore.h"
#include "Fuzzer/ObjectPool.h"
#include "Fuzzer/TestCaseDeserializer.h"
#include "Fuzzer/TestCaseSynthesiser.h"
#include "Fuzzer/SynthesisBuilder.h"
#include "Fuzzer/JavaScriptSynthesisBuilder.h"
#include "Fuzzer/NodejsSynthesisBuilder.h"
#include "Fuzzer/ChromeSynthesisBuilder.h"

#include "json/json.hpp"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

namespace caf {

namespace {

std::unique_ptr<char[]> DuplicateString(const char* s) {
  auto buffer = caf::make_unique<char[]>(std::strlen(s) + 1);
  std::strcpy(buffer.get(), s);
  return buffer;
}

} // namespace <anonymous>

class CalibrateCommand : public Command {
public:
  void SetupArgs(CLI::App &app) override {
    app.add_option("-s", _opt.CAFStorePath, "Path to the cafstore.json file")
        ->required()
        ->check(CLI::ExistingFile);
    app.add_option("-t,--target", _opt.TargetName,
        "The synthesis target. Available targets: js, nodejs, chrome")
        ->default_val("js");
    app.add_option("-e,--exec", _opt.ExecutableName, "Path to the executable file")
        ->required()
        ->check(CLI::ExistingFile);
    app.add_option("-X", _opt.ExecutableArgs, "Arguments to the executable file");
    app.add_option("tc", _opt.TestCaseFiles, "Paths to the test case files")
        ->check(CLI::ExistingFile);
  }

  int Execute(CLI::App &app) override {
    auto store = LoadCAFStore();
    if (!store) {
      PRINT_ERR_AND_EXIT("failed to load CAF metadata store");
    }

    auto pool = caf::make_unique<ObjectPool>();

    char jsFileName[32];
    std::strcpy(jsFileName, "/tmp/caf_XXXXXX");
    mkstemp(jsFileName);

    std::unordered_map<int, int> signalCounter;
    for (const auto& tcFile : _opt.TestCaseFiles) {
      if (tcFile.find("README.txt") != std::string::npos) {
        continue;
      }

      std::unique_ptr<SynthesisBuilder> synthesisBuilder;
      if (_opt.TargetName == "js") {
        synthesisBuilder = caf::make_unique<JavaScriptSynthesisBuilder>(*store);
      } else if(_opt.TargetName == "nodejs") {
        synthesisBuilder = caf::make_unique<NodejsSynthesisBuilder>(*store);
      } else if(_opt.TargetName == "chrome") {
        synthesisBuilder = caf::make_unique<ChromeSynthesisBuilder>(*store);
      }

      TestCaseSynthesiser synthesiser { *store, *synthesisBuilder };

      std::ifstream tcFileStream { tcFile };
      if (!tcFileStream) {
        PRINT_LAST_OS_ERR("cannot open test case file");
        continue;
      }

      StlInputStream tcFileInputStream { tcFileStream };
      TestCaseDeserializer de { *pool, tcFileInputStream };
      auto tc = de.Deserialize();

      synthesiser.Synthesis(tc);
      auto code = synthesiser.GetCode();

      if (truncate(jsFileName, 0) == -1) {
        PRINT_LAST_OS_ERR_AND_EXIT("truncate failed");
      }

      std::ofstream jsFileStream { jsFileName };
      if (jsFileStream.fail()) {
        PRINT_LAST_OS_ERR("failed to open output JavaScript file");
        continue;
      }

      jsFileStream << code;
      jsFileStream.close();

      std::vector<std::string> args;
      args.reserve(_opt.ExecutableArgs.size() + 2);
      args.push_back(_opt.ExecutableName);
      for (const auto& a : _opt.ExecutableArgs) {
        args.push_back(a);
      }
      args.push_back(jsFileName);

      auto sig = ExecuteProgram(args);
      ++signalCounter[sig];

      std::cout << tcFile << ": ";
      if (sig == 0) {
        std::cout << "no crash";
      } else {
        std::cout << strsignal(sig);
      }
      std::cout << std::endl;
    }

    std::cout << std::endl;
    std::cout << "========== OVERVIEW ==========" << std::endl;
    for (const auto& i : signalCounter) {
      auto sig = i.first;
      auto count = i.second;
      if (sig == 0) {
        std::cout << "\tno crash: ";
      } else {
        std::cout << "\t" << strsignal(sig) << ": ";
      }
      std::cout << count << std::endl;
    }

    return 0;
  }

private:
  struct Options {
    std::string CAFStorePath;
    std::string SignalName;
    std::string TargetName;
    std::string ExecutableName;
    std::vector<std::string> ExecutableArgs;
    std::vector<std::string> TestCaseFiles;
  }; // struct Options

  Options _opt;

  std::unique_ptr<CAFStore> LoadCAFStore() const {
    std::ifstream stream { _opt.CAFStorePath };
    if (stream.fail()) {
      PRINT_LAST_OS_ERR_AND_EXIT("failed to open CAF metadata database file");
    }

    nlohmann::json json;
    stream >> json;

    auto store = caf::make_unique<CAFStore>();
    store->Load(json);
    return store;
  }

  int ExecuteProgram(const std::vector<std::string>& args) const {
    auto pid = fork();
    if (pid < 0) {
      PRINT_LAST_OS_ERR_AND_EXIT("fork failed");
    }

    if (pid == 0) {
      std::vector<std::unique_ptr<char[]>> nativeArgsOwner;
      std::vector<char *> nativeArgs;
      nativeArgsOwner.reserve(args.size());
      nativeArgs.reserve(args.size());

      for (const auto& i : args) {
        nativeArgsOwner.push_back(DuplicateString(i.c_str()));
        nativeArgs.push_back(nativeArgsOwner.back().get());
      }

      nativeArgs.push_back(nullptr);

      auto devNull = open("/dev/null", O_WRONLY);
      if (devNull == -1) {
        PRINT_LAST_OS_ERR_AND_EXIT("open failed");
      }

      if (dup2(devNull, STDOUT_FILENO) == -1) {
        PRINT_LAST_OS_ERR_AND_EXIT("dup2 failed");
      }
      if (dup2(devNull, STDERR_FILENO) == -1) {
        PRINT_LAST_OS_ERR_AND_EXIT("dup2 failed");
      }

      execvp(nativeArgs[0], nativeArgs.data());

      PRINT_LAST_OS_ERR_AND_EXIT("execvp failed");
    }

    int status;
    if (waitpid(pid, &status, 0) == -1) {
      PRINT_LAST_OS_ERR_AND_EXIT("waitpid failed");
    }

    if (WIFSIGNALED(status)) {
      return WTERMSIG(status);
    } else {
      return 0;
    }
  }
}; // class CalibrateCommand

static RegisterCommand<CalibrateCommand> X {
    "calibrate", "Calibrate crashing test cases" };

} // namespace caf
