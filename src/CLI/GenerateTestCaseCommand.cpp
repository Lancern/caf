#include "Command.h"
#include "Diagnostics.h"
#include "RegisterCommand.h"
#include "Infrastructure/Memory.h"
#include "Infrastructure/Random.h"
#include "Infrastructure/Stream.h"
#include "Basic/CAFStore.h"
#include "Basic/JsonDeserializer.h"
#include "Fuzzer/Corpus.h"
#include "Fuzzer/TestCaseGenerator.h"
#include "Fuzzer/TestCaseSerializer.h"

#include <unistd.h>

#include <chrono>
#include <fstream>
#include <string>

namespace caf {

namespace {

/**
 * @brief Change the working directory of the calling process to the given directory. If the given
 * directory does not exist, then it will be created.
 *
 * This function will terminate the calling process if any errors happen.
 *
 * @param dir the path to the target directory.
 */
void ChangeWorkingDirectory(const char* dir) {
  int ret = chdir(dir);
  if (ret == ENOENT) {
    ret = mkdir(dir, 0777);
    if (ret != 0) {
      PRINT_LAST_OS_ERR_AND_EXIT_FMT("failed to create directory \"%s\"", dir);
    }
  }
  if (ret != 0) {
    PRINT_LAST_OS_ERR_AND_EXIT("failed to chdir to output directory");
  }
}

} // namespace <anonymous>

class GenerateTestCaseCommand : public Command {
public:
  virtual void SetupArgs(CLI::App& app) override {
    app.add_option("-s", _opts.storeFile, "Path to the cafstore.json file")
        ->check(CLI::ExistingFile)
        ->required();
    app.add_option("-o", _opts.outputDir, "Path to the output directory")
        ->required();
    app.add_option("-n", _opts.n, "Number of test cases to generate")
        ->default_val(1)
        ->check(CLI::PositiveNumber);
    app.add_option("-c", _opts.maxCalls, "Maximum number of calls to generate in each test case")
        ->default_val(5)
        ->check(CLI::PositiveNumber);
    app.add_option("--seed", _opts.seed, "Initial seed for the random number generator")
        ->check(CLI::Number);
    app.add_flag("--silence", _opts.silence, "Silent all informative log output");
  }

  virtual int Execute(CLI::App& app) override {
    if (!app.count("--seed")) {
      _opts.seed = static_cast<int>(
          std::chrono::high_resolution_clock::now().time_since_epoch().count());
    }

    if (!_opts.silence) {
      std::cout << "Loading CAF store from file \"" << _opts.storeFile << "\"..." << std::endl;
    }

    std::ifstream storeFile { _opts.storeFile };
    if (storeFile.fail()) {
      PRINT_LAST_OS_ERR_AND_EXIT_FMT("failed to open file \"%s\"", _opts.storeFile.c_str());
    }

    JsonDeserializer storeFileReader;
    auto store = storeFileReader.DeserializeCAFStoreFrom(storeFile);
    if (!store) {
      PRINT_ERR_AND_EXIT("bad store file content");
    }

    storeFile.close();
    ChangeWorkingDirectory(_opts.outputDir.c_str());

    auto corpus = caf::make_unique<CAFCorpus>(std::move(store));
    Random<> rnd;
    rnd.seed(_opts.seed);

    TestCaseGenerator gen { corpus.get(), rnd };
    TestCaseSerializer writer;
    for (auto tci = 0; tci < _opts.n; ++tci) {
      if (!_opts.silence) {
        std::cout << "Generating test case #" << tci << std::endl;
      }

      auto tc = gen.GenerateTestCase(_opts.maxCalls);

      std::string outputFileName = "seed";
      outputFileName.append(std::to_string(tci));
      outputFileName.append(".bin");

      std::ofstream outputFile { outputFileName };
      if (outputFile.fail()) {
        PRINT_LAST_OS_ERR_AND_EXIT_FMT(
            "failed to create output file \"%s\"", outputFileName.c_str());
      }

      StdStreamAdapter<std::ofstream> outputFileAdapter { outputFile };
      writer.Write(outputFileAdapter, *tc);
    }

    if (!_opts.silence) {
      std::cout << "Done." << std::endl;
    }

    return 0;
  }

private:
  struct Opts {
    std::string storeFile; // Path to the cafstore.json file
    std::string outputDir; // Path to the output directory
    int n; // Number of test cases to generate
    int maxCalls; // Maximum number of API calls generated in each test case
    int seed; // Initial seed for the random number generator
    bool silence; // Silent all informative output.
  }; // struct Opts

  Opts _opts;
}; // class GenerateTestCaseCommand

static RegisterCommand<GenerateTestCaseCommand> X { "gen", "Generate test cases randomly" };

} // namespace caf
