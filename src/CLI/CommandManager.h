#ifndef CAF_COMMAND_MANAGER_H
#define CAF_COMMAND_MANAGER_H

#include <memory>
#include <vector>
#include <unordered_map>

namespace CLI {
class App;
}; // namespace CLI

namespace caf {

class Command;

/**
 * @brief Manage registered CLI commands.
 *
 */
class CommandManager {
public:
  explicit CommandManager() = default;

  CommandManager(const CommandManager &) = delete;
  CommandManager(CommandManager &&) noexcept;

  CommandManager& operator=(const CommandManager &) = delete;
  CommandManager& operator=(CommandManager &&);

  /**
   * @brief Register the given CLI command.
   *
   * @param cmd the CLI command to register.
   * @param name the name of the command.
   * @param desc the description of the command.
   */
  void Register(std::unique_ptr<Command> cmd, const char* name, const char* desc);

  /**
   * @brief Call all commands to setup their command line arguments.
   *
   * @param app the root command line settings for the CLI utility application.
   */
  void SetupArgs(CLI::App& app);

  /**
   * @brief Get the CLI command corresponding to the given command line argument settings.
   *
   * @param app the command line argument settings.
   * @return Command* the CLI command. If no such CLI command exits, returns nullptr.
   */
  Command* GetCommandOfApp(const CLI::App* app) const;

  /**
   * @brief Get the singleton CommandManager object.
   *
   * @return CommandManager* the singleton CommandManager object.
   */
  static CommandManager* GetSingleton();

private:
  struct CommandInfo {
    explicit CommandInfo(const char* name, const char* desc, std::unique_ptr<Command> cmd)
      : name(name), desc(desc), cmd(std::move(cmd))
    { }

    CommandInfo(const CommandInfo &) = delete;
    CommandInfo(CommandInfo &&) noexcept;

    CommandInfo& operator=(const CommandInfo &) = delete;
    CommandInfo& operator=(CommandInfo &&);

    ~CommandInfo();

    const char* name;
    const char* desc;
    std::unique_ptr<Command> cmd;
  }; // struct CommandInfo

  std::vector<CommandInfo> _cmds;
  std::unordered_map<const CLI::App *, Command *> _args;
}; // class CommandManager

} // namespace caf

#endif
