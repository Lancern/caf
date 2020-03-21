#include "Command.h"
#include "RegisterCommand.h"
#include "Diagnostics.h"
#include "Printer.h"
#include "TestCaseDumper.h"
#include "Infrastructure/Memory.h"
#include "Infrastructure/Stream.h"
#include "Basic/CAFStore.h"
#include "Fuzzer/ObjectPool.h"
#include "Fuzzer/TestCaseDeserializer.h"

#include "json/json.hpp"

#include <fstream>
#include <memory>

namespace caf {

class ShowCommand : public Command {
public:
  void SetupArgs(CLI::App &app) override {
    app.add_option("-s", _opts.StoreFileName, "Path to the cafstore.json file")
        ->required()
        ->check(CLI::ExistingFile);
    app.add_flag("-d,--demangle", _opts.Demangle, "Demangle symbol names before printing");
    app.add_flag("--no-color", _opts.NoColor, "Disable coloring output");
    app.add_option("tc", _opts.TestCaseFileName, "Path to the test case file")
        ->required()
        ->check(CLI::ExistingFile);
  }

  int Execute(CLI::App &app) override {
    std::ifstream storeFile { _opts.StoreFileName };
    if (storeFile.fail()) {
      PRINT_LAST_OS_ERR_AND_EXIT("failed to load CAF store file");
    }

    nlohmann::json json;
    storeFile >> json;
    auto store = caf::make_unique<CAFStore>(json);
    auto pool = caf::make_unique<ObjectPool>();
    storeFile.close();

    std::ifstream tcFile { _opts.TestCaseFileName };
    if (tcFile.fail()) {
      PRINT_LAST_OS_ERR_AND_EXIT("failed to load test case file");
    }
    StlInputStream fileStream { tcFile };
    TestCaseDeserializer de { *pool, fileStream };
    auto tc = de.Deserialize();

    Printer printer { std::cout };
    printer.SetColorOn(!_opts.NoColor);

    TestCaseDumper dumper { *store, printer };
    dumper.SetDemangle(_opts.Demangle);

    dumper.Dump(tc);

    printer << Printer::endl;
    return 0;
  }

private:
  struct Opts {
    std::string StoreFileName;
    std::string TestCaseFileName;
    bool Demangle = false;
    bool NoColor = false;
  }; // struct Opts

  Opts _opts;
}; // class ShowCommand

static RegisterCommand<ShowCommand> X {
    "show", "Display a test case in a human readable form." };

} // namespace caf
