#include "Command.h"
#include "RegisterCommand.h"
#include "Diagnostics.h"
#include "CAFConfig.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <utility>
#include <string>
#include <vector>

#include <unistd.h>
#include <ftw.h>

#define AFL_EXECUTABLE_NAME "afl-fuzz"

namespace caf {

namespace {

bool IsValidExecutable(const char* path) {
  return access(path, X_OK) == 0;
}

std::string LookupAflExecutable(std::string dir) {
  if (!dir.empty() && dir[dir.size() - 1] != '/') {
    dir.push_back('/');
  }
  dir.append(AFL_EXECUTABLE_NAME);
  if (IsValidExecutable(dir.c_str())) {
    return dir;
  }

  return std::string();
}

std::string LookupAflExecutable() {
  if (IsValidExecutable(AFL_EXECUTABLE_NAME)) {
    return std::string(AFL_EXECUTABLE_NAME);
  }

  auto pathVar = std::getenv("PATH");
  if (!pathVar) {
    return std::string();
  }

  while (*pathVar++) {
    auto pathEnd = std::strpbrk(pathVar, ":");
    if (pathEnd == pathVar) {
      continue;
    }
    if (pathEnd) {
      *pathEnd = 0;
    }

    auto dir = std::string(pathVar);
    if (pathEnd) {
      *pathEnd = ':';
    }

    auto path = LookupAflExecutable(std::move(dir));
    if (!path.empty()) {
      return path;
    }

    if (!pathEnd) {
      break;
    }
    pathVar = pathEnd + 1;
  }

  return std::string();
}

char* DuplicateString(const char* s) {
  auto len = std::strlen(s);
  auto buffer = new char[len + 1];
  std::strcpy(buffer, s);
  return buffer;
}

} // namespace <anonymous>

class FuzzCommand : public Command {
public:
  void SetupArgs(CLI::App &app) override {
    app.add_option("-s", _opts.StoreFileName, "Path to the cafstore.json file")
        ->required()
        ->check(CLI::ExistingFile);
    app.add_option("-d", _opts.SeedDir, "Path to the seed directory")
        ->required()
        ->check(CLI::ExistingDirectory);
    app.add_option("-o", _opts.FindingsDir, "Path to the AFL findings directory")
        ->required();
    app.add_option("--afl", _opts.AflExecutable, "Path to the AFLplusplus executable")
        ->check(CLI::ExistingFile);
    app.add_flag("--verbose", _opts.Verbose, "Enable verbose output");
    app.add_option("target", _opts.Target, "The target executable and its params")
        ->required();
  }

  int Execute(CLI::App &app) override {
    if (_opts.AflExecutable.empty()) {
      _opts.AflExecutable = LookupAflExecutable();
      if (_opts.AflExecutable.empty()) {
        PRINT_ERR_AND_EXIT("Cannot locate valid AFLplusplus executables.");
      }
    }

    if (_opts.Verbose) {
      std::cout << "AFLplusplus located at " << _opts.AflExecutable << std::endl;
    }

    std::string storeVar = "CAF_STORE=";
    storeVar.append(_opts.StoreFileName);
    if (_opts.Verbose) {
      std::cout << "export CAF_STORE=" << _opts.StoreFileName << std::endl;
    }

    std::string mutatorLibVar = "AFL_CUSTOM_MUTATOR_LIBRARY=";
    mutatorLibVar.append(CAF_LIB_DIR);
    mutatorLibVar.append("/libCAFMutator.so");
    if (_opts.Verbose) {
      std::cout << "export AFL_CUSTOM_MUTATOR_LIBRARY=" << CAF_LIB_DIR << "/libCAFMutator.so";
    }

    std::vector<char *> aflEnv;
    for (auto e = environ; *e; ++e) {
      aflEnv.push_back(*e);
    }
    aflEnv.push_back(DuplicateString(storeVar.c_str()));
    aflEnv.push_back(DuplicateString(mutatorLibVar.c_str()));
    aflEnv.push_back(DuplicateString("AFL_CUSTOM_MUTATOR_ONLY=1"));
    aflEnv.push_back(nullptr);

    std::vector<char *> aflArgs {
      DuplicateString(_opts.AflExecutable.c_str()),
      DuplicateString("-i"),
      DuplicateString(_opts.SeedDir.c_str()),
      DuplicateString("-o"),
      DuplicateString(_opts.FindingsDir.c_str())
    };
    for (const auto& arg : _opts.Target) {
      aflArgs.push_back(DuplicateString(arg.c_str()));
    }
    aflArgs.push_back(nullptr);

    if (_opts.Verbose) {
      std::cout << "Launching AFLplusplus:" << std::endl << "\t";
      for (auto arg : aflArgs) {
        if (std::strpbrk(arg, " ")) {
          std::cout << std::quoted(arg) << ' ';
        } else {
          std::cout << arg << ' ';
        }
      }
      std::cout << std::endl;
    }

    execve(aflArgs[0], aflArgs.data(), aflEnv.data());
    PRINT_LAST_OS_ERR_AND_EXIT("execve failed");

    return 1;
  }

private:
  struct Opts {
    std::string StoreFileName;
    std::string SeedDir;
    std::string FindingsDir;
    std::string AflExecutable;
    std::vector<std::string> Target;
    bool Verbose;
  }; // struct Opts

  Opts _opts;
}; // class FuzzCommand

static RegisterCommand<FuzzCommand> X { "fuzz", "Fuzz a target using CAF" };

} // namespace caf
