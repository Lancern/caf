#ifndef CAF_FUNCTION_DATABASE_H
#define CAF_FUNCTION_DATABASE_H

#include <cassert>
#include <unordered_map>

namespace caf {

/**
 * @brief Provide a mapping from function ID to function objects.
 *
 * @tparam TargetTraits the trait type describing target's type system.
 */
template <typename TargetTraits>
class FunctionDatabase {
public:
  /**
   * @brief Construct a new FunctionDatabase object.
   *
   */
  explicit FunctionDatabase() = default;

  FunctionDatabase(const FunctionDatabase<TargetTraits> &) = delete;
  FunctionDatabase(FunctionDatabase<TargetTraits> &&) noexcept = default;

  FunctionDatabase<TargetTraits>& operator=(const FunctionDatabase<TargetTraits> &) = delete;
  FunctionDatabase<TargetTraits>& operator=(FunctionDatabase<TargetTraits> &&) = default;

  /**
   * @brief Add a new function to this mapping.
   *
   * The new function ID should not exist in this mapping.
   *
   * @param funcId ID of the function.
   * @param function the function object.
   */
  void AddFunction(uint32_t funcId, typename TargetTraits::FunctionType function) {
    assert(_funcs.find(funcId) == _funcs.end() && "The same function ID already exists.");
    _funcs.emplace(funcId, function);
  }

  /**
   * @brief Get the function object associated with the given function ID.
   *
   * @param funcId the function ID.
   * @return TargetTraits::FunctionType the function object.
   */
  typename TargetTraits::FunctionType GetFunction(uint32_t funcId) const {
    return _funcs.at(funcId);
  }

private:
  std::unordered_map<uint32_t, typename TargetTraits::FunctionType> _funcs;
}; // class FunctionDatabase

} // namespace caf

#endif
