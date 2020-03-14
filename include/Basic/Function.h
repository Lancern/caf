#ifndef CAF_FUNCTION_H
#define CAF_FUNCTION_H

#include "json/json.hpp"

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
   * @brief Deserialize a Function object from the given JSON container.
   *
   * @param json the JSON container.
   */
  explicit Function(const nlohmann::json& json);

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

  /**
   * @brief Serialize this Function object into JSON form.
   *
   * @return nlohmann::json the JSON form of this Function object.
   */
  nlohmann::json ToJson() const;

private:
  FunctionIdType _id;
  std::string _name;
}; // class Function

} // namespace caf

#endif
