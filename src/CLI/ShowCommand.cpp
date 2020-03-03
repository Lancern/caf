#include "Command.h"
#include "RegisterCommand.h"
#include "Diagnostics.h"
#include "Printer.h"
#include "TestCaseDumper.h"
#include "Infrastructure/Memory.h"
#include "Infrastructure/Stream.h"
#include "Basic/JsonDeserializer.h"
#include "Fuzzer/Corpus.h"
#include "Fuzzer/TestCaseDeserializer.h"

#include <fstream>
#include <memory>

namespace caf {

class ShowCommand : public Command {
public:
  void SetupArgs(CLI::App &app) override {
    app.add_option("-s", _opts.StoreFileName, "Path to the cafstore.json file")
        ->required()
        ->check(CLI::ExistingFile);
    app.add_option("tc", _opts.TestCaseFileName, "Path to the test case file")
        ->required()
        ->check(CLI::ExistingFile);
  }

  int Execute(CLI::App &app) override {
    std::ifstream storeFile { _opts.StoreFileName };
    if (storeFile.fail()) {
      PRINT_LAST_OS_ERR_AND_EXIT("failed to load CAF store file");
    }

    JsonDeserializer storeReader;
    auto store = storeReader.DeserializeCAFStoreFrom(storeFile);
    if (!store) {
      PRINT_ERR_AND_EXIT("failed to load CAF store data");
    }

    std::ifstream tcFile { _opts.TestCaseFileName };
    if (tcFile.fail()) {
      PRINT_LAST_OS_ERR_AND_EXIT("failed to load test case file");
    }
    StdStreamAdapter<std::ifstream> tcFileWrapper { tcFile };

    auto corpus = caf::make_unique<CAFCorpus>(std::move(store));

    TestCaseDeserializer tcReader { corpus.get() };
    auto tc = tcReader.Read(tcFileWrapper);

    Printer printer { std::cout };
    TestCaseDumper dumper { printer };
    dumper.Dump(*tc);

    return 0;
  }

private:
  struct Opts {
    std::string StoreFileName;
    std::string TestCaseFileName;
  }; // struct Opts

  Opts _opts;
}; // class ShowCommand

static RegisterCommand<ShowCommand> X {
    "show", "Display a test case in a human readable form." };

} // namespace caf
