#include "Infrastructure/Memory.h"
#include "Command.h"
#include "CommandManager.h"

#include "CLI11/CLI11.hpp"

#include <memory>

namespace caf {

namespace {

std::unique_ptr<CommandManager> _singleton;

} // namepace <anonymous>

CommandManager::CommandInfo::CommandInfo(CommandInfo &&) noexcept = default;

CommandManager::CommandInfo& CommandManager::CommandInfo::operator=(CommandInfo &&) = default;

CommandManager::CommandInfo::~CommandInfo() = default;

void CommandManager::Register(std::unique_ptr<Command> cmd, const char* name, const char* desc) {
  CommandInfo info { name, desc, std::move(cmd) };
  _cmds.push_back(std::move(info));
}

void CommandManager::SetupArgs(CLI::App& app) {
  for (const auto& cmd : _cmds) {
    auto cmdArgs = app.add_subcommand(cmd.name, cmd.desc);
    cmd.cmd->SetupArgs(*cmdArgs);
    _args[cmdArgs] = cmd.cmd.get();
  }
}

Command* CommandManager::GetCommandOfApp(const CLI::App* app) const {
  auto i = _args.find(app);
  if (i == _args.end()) {
    return nullptr;
  }
  return i->second;
}

CommandManager* CommandManager::GetSingleton() {
  if (!_singleton) {
    _singleton = caf::make_unique<CommandManager>();
  }
  return _singleton.get();
}

} // namespace caf
