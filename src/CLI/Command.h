#ifndef CAF_COMMAND_H
#define CAF_COMMAND_H

#include "CLI11/CLI11.hpp"

namespace caf {

/**
 * @brief Abstract base class for all CAF CLI commands.
 *
 */
class Command {
public:
  /**
   * @brief Construct a new Command object.
   *
   */
  explicit Command() = default;

  Command(const Command &) = delete;
  Command(Command &&) noexcept = default;

  Command& operator=(const Command &) = delete;
  Command& operator=(Command &&) = default;

  virtual ~Command() = default;

  /**
   * @brief When overridden in derived classes, setup command line arguments for this command.
   *
   * @param app the command line settings for this command.
   */
  virtual void SetupArgs(CLI::App& app) const = 0;

  /**
   * @brief When overridden in derived classes, execute this command.
   *
   * @param app the command line args for this command.
   * @return int the exit code of this command.
   */
  virtual int Execute(CLI::App& app) = 0;
}; // class Command

} // namespace caf

#endif
