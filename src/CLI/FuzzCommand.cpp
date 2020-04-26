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

#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
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

pid_t ExecuteCommand(
    const std::vector<char *>& args,
    const std::vector<char *>& envs,
    bool noOutput = false) {
  auto pid = fork();
  if (pid < 0) {
    PRINT_LAST_OS_ERR_AND_EXIT("fork failed");
  }

  if (pid > 0) {
    return pid;
  }

  if (noOutput) {
    auto devNull = open("/dev/null", O_WRONLY);
    if (devNull == -1) {
      PRINT_LAST_OS_ERR_AND_EXIT("open /dev/null failed");
    }

    if (dup2(devNull, STDOUT_FILENO) == -1) {
      PRINT_LAST_OS_ERR_AND_EXIT("dup2 failed");
    }
    if (dup2(devNull, STDERR_FILENO) == -1) {
      PRINT_LAST_OS_ERR_AND_EXIT("dup2 failed");
    }
  }

  // We're in child process.
  execve(args[0], args.data(), envs.data());
  PRINT_LAST_OS_ERR_AND_EXIT("exec failed");

  // Make some old compiler happy.
  return 0;
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
    app.add_flag("--resume", _opts.Resume, "Enable AFLplusplus auto resume");
    app.add_option("-n", _opts.Parallelization, "Number of parallel afl-fuzz instances to run")
        ->check(CLI::PositiveNumber)
        ->default_val(1);
    app.add_flag("--dry", _opts.DryRun, "Only construct arguments to AFL, do not actually run AFL");
    app.add_option("--san-exec", _opts.SanitizedTarget, "Path to the sanitized executable file");
    app.add_flag("--quiet", _opts.Quiet, "Redirect AFL's output to /dev/null");
    app.add_option("-X", _opts.AFLArgs, "Arguments passed to AFL executable");
    app.add_flag("--verbose", _opts.Verbose, "Enable verbose output");
    app.add_option("target", _opts.Target, "The target executable and its params")
        ->required();
  }

  int Execute(CLI::App &app) override {
    _opts.Verbose = _opts.Verbose || _opts.DryRun;
    _opts.Quiet = _opts.Quiet || (_opts.Parallelization > 1);
    if (_opts.SanitizedTarget.empty()) {
      _opts.SanitizedTarget = _opts.Target.at(0);
    }

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
    mutatorLibVar.append("/libCAFMutatorForChrome.so");
    if (_opts.Verbose) {
      std::cout << "export AFL_CUSTOM_MUTATOR_LIBRARY="
                << CAF_LIB_DIR << "/libCAFMutatorForChrome.so"
                << std::endl;
    }

    std::vector<char *> aflEnv;
    for (auto e = environ; *e; ++e) {
      aflEnv.push_back(*e);
    }
    aflEnv.push_back(DuplicateString(storeVar.c_str()));
    aflEnv.push_back(DuplicateString(mutatorLibVar.c_str()));
    aflEnv.push_back(DuplicateString("AFL_CUSTOM_MUTATOR_ONLY=1"));
    if (_opts.Resume) {
      aflEnv.push_back(DuplicateString("AFL_AUTORESUME=1"));
    }
    aflEnv.push_back(nullptr);

    std::vector<char *> aflArgs {
      DuplicateString(_opts.AflExecutable.c_str()),
      DuplicateString("-o"),
      DuplicateString(_opts.FindingsDir.c_str())
    };
    aflArgs.push_back(DuplicateString("-i"));
    if (_opts.Resume) {
      aflArgs.push_back(DuplicateString("-"));
    } else {
      aflArgs.push_back(DuplicateString(_opts.SeedDir.c_str()));
    }

    int parallelArg = -1;
    if (_opts.Parallelization > 1) {
      parallelArg = static_cast<int>(aflArgs.size());
      aflArgs.push_back(nullptr);
      aflArgs.push_back(nullptr);
    }

    for (const auto& arg : _opts.AFLArgs) {
      aflArgs.push_back(DuplicateString(arg.c_str()));
    }
    int execArg = aflArgs.size();
    aflArgs.push_back(DuplicateString(_opts.SanitizedTarget.c_str()));
    for (size_t i = 1; i < _opts.Target.size(); ++i) {
      const auto& arg = _opts.Target.at(i);
      aflArgs.push_back(DuplicateString(arg.c_str()));
    }
    aflArgs.push_back(nullptr);

    std::vector<pid_t> children;
    children.reserve(_opts.Parallelization);
    for (auto i = 0; i < _opts.Parallelization; ++i) {
      if (parallelArg != -1) {
        aflArgs[parallelArg] = (i == 0)
            ? DuplicateString("-M")
            : DuplicateString("-S");
        std::string fuzzerName = "fuzzer";
        fuzzerName.append(std::to_string(i));
        aflArgs[parallelArg + 1] = DuplicateString(fuzzerName.c_str());
      }

      if (_opts.Verbose) {
        std::cout << "Launching AFLplusplus:" << std::endl << "\t";
        for (auto arg : aflArgs) {
          if (!arg) {
            break;
          }
          if (std::strpbrk(arg, " ")) {
            std::cout << std::quoted(arg) << ' ';
          } else {
            std::cout << arg << ' ';
          }
        }
        if (_opts.Quiet) {
          std::cout << "1>/dev/null 2>/dev/null";
        }
        std::cout << std::endl;
      }

      if (!_opts.DryRun) {
        children.push_back(ExecuteCommand(aflArgs, aflEnv, _opts.Quiet));
        std::cout << "Fuzzer #" << i << " has started, pid = " << children.back() << std::endl;
      }

      if (i == 0 && !_opts.SanitizedTarget.empty()) {
        aflArgs[execArg] = DuplicateString(_opts.Target[0].c_str());
      }
    }

    if (_opts.DryRun) {
      return 0;
    }

    auto ok = true;
    for (auto childId : children) {
      int status;
      if (waitpid(childId, &status, 0) == -1) {
        PRINT_LAST_OS_ERR("waitpid failed");
        ok = false;
      }
      if (WIFEXITED(status)) {
        std::cout << "Fuzzer " << childId << " has exited. Exit status = "
                  << WEXITSTATUS(status) << std::endl;
      } else if (WIFSIGNALED(status)) {
        std::cerr << "Fuzzer " << childId << " has terminated. Signal is "
                  << strsignal(WTERMSIG(status))
                  << " (" << WTERMSIG(status) << ")"
                  << std::endl;
      }
    }

    return static_cast<int>(!ok);
  }

private:
  struct Opts {
    explicit Opts()
      : StoreFileName(), SeedDir(), FindingsDir(), AflExecutable(), Target(), AFLArgs(),
        Parallelization(1), Resume(false), DryRun(false), Verbose(false), Quiet(false)
    { }

    std::string StoreFileName;
    std::string SeedDir;
    std::string FindingsDir;
    std::string AflExecutable;
    std::vector<std::string> Target;
    std::string SanitizedTarget;
    std::vector<std::string> AFLArgs;
    int Parallelization;
    bool Resume;
    bool DryRun;
    bool Verbose;
    bool Quiet;
  }; // struct Opts

  Opts _opts;
}; // class FuzzCommand

static RegisterCommand<FuzzCommand> X { "fuzz", "Fuzz a target using CAF" };

} // namespace caf
