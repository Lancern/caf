#include "Command.h"
#include "Diagnostics.h"
#include "RegisterCommand.h"

#include "Infrastructure/Stream.h"
#include "Infrastructure/Memory.h"

#include "Basic/CAFStore.h"
#include "Fuzzer/TestCase.h"
#include "Fuzzer/ObjectPool.h"
#include "Fuzzer/TestCaseDeserializer.h"
#include "Fuzzer/TestCaseSynthesiser.h"
#include "Fuzzer/SynthesisBuilder.h"
#include "Fuzzer/JavaScriptSynthesisBuilder.h"
#include "Fuzzer/NodejsSynthesisBuilder.h"

#include "json/json.hpp"

#include <string>
#include <fstream>
#include <unordered_map>
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace caf {

namespace {

void WriteTextToFile(const char* path, const char* text) {
  std::ofstream output { path };
  if (output.fail()) {
    PRINT_LAST_OS_ERR_AND_EXIT("cannot open file");
  }
  output << text;
}

bool CreateOutputDirectory(const char* path) {
  struct stat s;
  if (stat(path, &s) == -1) {
    if (errno == ENOENT) {
      if (mkdir(path, 0755) == -1) {
        PRINT_LAST_OS_ERR_AND_EXIT("mkdir failed");
      }
      return true;
    } else {
      PRINT_LAST_OS_ERR_AND_EXIT("stat failed");
    }
  } else {
    return S_ISDIR(s.st_mode);
  }
}

} // namespace <anonymous>

class SynthesisCommand : public Command {
public:
  explicit SynthesisCommand() = default;

  void SetupArgs(CLI::App &app) override {
    app.add_option("-s", _opts.CAFStorePath, "Path to the cafstore.json file")
        ->required();
    app.add_option("-t,--target", _opts.Target, "Name of the target. Available targets: js, nodejs")
        ->default_val("js");
    app.add_option("-o,--out", _opts.Output, "Path to the output directory or file");
    app.add_option("tc", _opts.TestCasePaths, "Paths to the test case files")
        ->required();
  }

  int Execute(CLI::App &app) override {
    // Check target name.
    if (_opts.Target != "js" && _opts.Target != "nodejs") {
      PRINT_ERR_AND_EXIT_FMT("invalid target: %s", _opts.Target.c_str());
    }

    std::ifstream storeFile { _opts.CAFStorePath };
    if (storeFile.fail()) {
      PRINT_LAST_OS_ERR_AND_EXIT("failed to load CAF store file");
    }

    nlohmann::json json;
    storeFile >> json;
    auto store = caf::make_unique<CAFStore>();
    store->Load(json);
    auto pool = caf::make_unique<ObjectPool>();
    storeFile.close();

    if (_opts.TestCasePaths.size() > 1 && !CreateOutputDirectory(_opts.Output.c_str())) {
      PRINT_LAST_OS_ERR_AND_EXIT("cannot create output directory");
    }

    for (const auto& path : _opts.TestCasePaths) {
      std::ifstream tcFile { path };
      if (tcFile.fail()) {
        PRINT_LAST_OS_ERR_AND_EXIT("failed to load test case file");
      }
      StlInputStream fileStream { tcFile };
      TestCaseDeserializer de { *pool, fileStream };
      auto tc = de.Deserialize();

      std::unique_ptr<SynthesisBuilder> synthesisBuilder;
      if (_opts.Target == "js") {
        synthesisBuilder = caf::make_unique<JavaScriptSynthesisBuilder>(*store);
      } else {
        // Target is nodejs.
        synthesisBuilder = caf::make_unique<NodejsSynthesisBuilder>(*store);
      }

      TestCaseSynthesiser syn { *store, *synthesisBuilder };
      syn.Synthesis(tc);

      auto code = syn.GetCode();
      if (_opts.Output.empty()) {
        std::cout << code << std::endl;
      } else {
        if (_opts.TestCasePaths.size() == 1) {
          WriteTextToFile(_opts.Output.c_str(), code.c_str());
        } else {
          auto outputPath = _opts.Output;
          if (!outputPath.empty() && outputPath.back() != '/') {
            outputPath.push_back('/');
          }
          auto sep = path.rfind('/');
          if (sep == std::string::npos) {
            outputPath.append(path);
          } else {
            outputPath.append(path.substr(sep + 1));
          }
          WriteTextToFile(outputPath.c_str(), code.c_str());
        }
      }
    }

    return 0;
  }

private:
  struct Opts {
    std::string CAFStorePath;
    std::string Target;
    std::string Output;
    std::vector<std::string> TestCasePaths;
  }; // struct Opts

  Opts _opts;
};

static RegisterCommand<SynthesisCommand> X {
    "synthesis", "Synthesis a test case to script form" };

} // namespace caf
