#ifndef CAF_FUNCTION_H
#define CAF_FUNCTION_H

#include <string>

namespace caf {

/**
 * @brief Type of API function IDs.
 *
 */
using FunctionIdType = uint32_t;

/**
 * @brief Represent an API function.
 *
 */
class Function {
public:
  /**
   * @brief Construct a new Function object.
   *
   * @param id ID of the function.
   * @param name name of the function.
   */
  explicit Function(uint64_t id, std::string name)
    : _id(id), _name(std::move(name))
  { }

  Function(const Function &) = delete;
  Function(Function &&) noexcept = default;

  Function& operator=(const Function &) = delete;
  Function& operator=(Function &&) = default;

  /**
   * @brief Get the ID of this function.
   *
   * @return FunctionIdType ID of this function.
   */
  FunctionIdType id() const { return _id; }

  /**
   * @brief Get the name of this function.
   *
   * @return const std::string& name of this function.
   */
  const std::string& name() const { return _name; }

private:
  FunctionIdType _id;
  std::string _name;
}; // class Function

} // namespace caf

#endif
