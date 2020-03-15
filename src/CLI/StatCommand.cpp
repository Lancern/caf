#include "Command.h"
#include "Diagnostics.h"
#include "RegisterCommand.h"
#include "Basic/CAFStore.h"

#include "json/json.hpp"

#include <fstream>
#include <iostream>
#include <string>

namespace caf {

namespace {

void DumpStatistics(const CAFStore::Statistics& stat) {
  std::cout << "========== CAF Store Statistics ==========" << std::endl;
  std::cout << "Number of API functions: " << stat.ApiFunctionsCount << std::endl;
  std::cout << "========== CAF Store Statistics ==========" << std::endl;
}

} // namespace <anonymous>

class StatCommand : public Command {
public:
  void SetupArgs(CLI::App &app) override {
    app.add_option("-s", _opts.storeFile, "Path to the cafstore.json file")
        ->required()
        ->check(CLI::ExistingFile);
  }

  int Execute(CLI::App &app) override {
    std::ifstream file { _opts.storeFile };
    if (file.fail()) {
      PRINT_LAST_OS_ERR_AND_EXIT_FMT("failed to open file \"%s\"", _opts.storeFile.c_str());
    }

    nlohmann::json json;
    file >> json;
    auto store = caf::make_unique<CAFStore>(json);
    file.close();

    auto stat = store->GetStatistics();
    DumpStatistics(stat);

    return 0;
  }

private:
  struct Opts {
    std::string storeFile; // Path to the cafstore.json file.
  }; // struct Opts

  Opts _opts;
}; // class StatCommand

static RegisterCommand<StatCommand> X {
    "stat", "Display statistical information about a cafstore.json file" };

} // namespace caf
