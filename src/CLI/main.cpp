#include "Command.h"
#include "CommandManager.h"

#include "CLI11/CLI11.hpp"

#include <iostream>

int main(int argc, char* argv[]) {
  auto cmdMgr = caf::CommandManager::GetSingleton();

  CLI::App app { "CAF CLI utility" };
  cmdMgr->SetupArgs(app);
  CLI11_PARSE(app, argc, argv);

  auto cmdArgs = app.get_subcommand();
  auto cmd = cmdMgr->GetCommandOfApp(cmdArgs);
  if (!cmd) {
    std::cerr << "error: no such subcommand." << std::endl;
    return 1;
  }

  return cmd->Execute(*cmdArgs);
}
