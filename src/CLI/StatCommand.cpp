#include "Command.h"
#include "Diagnostics.h"
#include "RegisterCommand.h"
#include "Basic/JsonDeserializer.h"

#include <fstream>
#include <iostream>
#include <string>

namespace caf {

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

    JsonDeserializer reader;
    auto store = reader.DeserializeCAFStoreFrom(file);
    if (!store) {
      PRINT_ERR_AND_EXIT_FMT("failed to load file \"%s\"", _opts.storeFile.c_str());
    }

    auto stat = store->CreateStatistics();
    stat.dump(std::cout);

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
