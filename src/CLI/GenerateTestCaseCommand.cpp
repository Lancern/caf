#include "Command.h"
#include "RegisterCommand.h"

namespace caf {

class GenerateTestCaseCommand : public Command {
public:
  virtual void SetupArgs(CLI::App& app) const override {
    // TODO: Implement GenerateTestCaseCommand::SetupArgs.
  }

  virtual int Execute(CLI::App& app) override {
    // TODO: Implement GenerateTestCaseCommand::Execute.
    return 0;
  }
}; // class GenerateTestCaseCommand

static RegisterCommand<GenerateTestCaseCommand> X { "gen", "Generate test cases randomly" };

} // namespace caf
