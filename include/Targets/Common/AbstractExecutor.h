#ifndef CAF_ABSTRACT_EXECUTOR_H
#define CAF_ABSTRACT_EXECUTOR_H

#include <cstdint>
#include <vector>

namespace caf {

/**
 * @brief Abstract base class for executing a single API function call.
 *
 * @tparam TargetTraits trait type describing the target's type system.
 */
template <typename TargetTraits>
class AbstractExecutor {
public:
  using ValueType = typename TargetTraits::ValueType;
  using FunctionType = typename TargetTraits::FunctionType;

  /**
   * @brief Destroy the AbstractExecutor object.
   *
   */
  virtual ~AbstractExecutor() = default;

  /**
   * @brief Invoke the given API function.
   *
   * @param function the function to call.
   * @param receiver the receiver.
   * @param isCtorCall whether this function call should be made as a constructor call.
   * @param args the arguments to the function.
   * @return ValueType the return value of the API function.
   */
  virtual ValueType Invoke(
      FunctionType function, ValueType receiver, bool isCtorCall, std::vector<ValueType>& args) = 0;

protected:
  /**
   * @brief Construct a new AbstractExecutor object.
   *
   */
  explicit AbstractExecutor() = default;

  AbstractExecutor(const AbstractExecutor<TargetTraits> &) = delete;
  AbstractExecutor(AbstractExecutor<TargetTraits> &&) noexcept = default;

  AbstractExecutor<TargetTraits>& operator=(const AbstractExecutor<TargetTraits> &) = delete;
  AbstractExecutor<TargetTraits>& operator=(AbstractExecutor<TargetTraits> &&) = default;
}; // class AbstractExecutor

} // namespace caf

#endif
