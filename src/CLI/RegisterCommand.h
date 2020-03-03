#ifndef CAF_REGISTER_COMMAND_H
#define CAF_REGISTER_COMMAND_H

#include "Infrastructure/Memory.h"
#include "Command.h"
#include "CommandManager.h"

#include <type_traits>

namespace caf {

/**
 * @brief Register the CLI command given by the template parameter.
 *
 * @tparam T the type of the CLI command. T must derive from Command and must be default
 * constructible.
 */
template <typename T>
struct RegisterCommand {
  static_assert(std::is_base_of<Command, T>::value, "T does not derive from Command.");
  static_assert(std::is_default_constructible<T>::value, "T is not default constructible.");

  /**
   * @brief Register the command given by the template argument to the command manager.
   *
   * @param name the name of the command.
   * @param desc the description of the command.
   */
  explicit RegisterCommand(const char* name, const char* desc) {
    auto cmd = caf::make_unique<T>();
    CommandManager::GetSingleton()->Register(std::move(cmd), name, desc);
  }
}; // struct RegisterCommand

} // namespace caf

#endif
