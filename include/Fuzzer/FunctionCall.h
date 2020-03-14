#ifndef CAF_FUNCTION_CALL_H
#define CAF_FUNCTION_CALL_H

#include "Basic/Function.h"
#include "Fuzzer/Value.h"

#include <utility>
#include <memory>
#include <iterator>
#include <vector>

namespace caf {

/**
 * @brief Represent an API function call in a test case.
 *
 */
class FunctionCall {
public:
  using Iterator = typename std::vector<Value *>::iterator;
  using ConstIterator = typename std::vector<Value *>::const_iterator;

  /**
   * @brief Construct a new FunctionCall object.
   *
   * @param funcId the function call ID.
   */
  explicit FunctionCall(FunctionIdType funcId)
    : _funcId(funcId)
  { }

  /**
   * @brief Get the function ID.
   *
   * @return FunctionIdType the function ID.
   */
  FunctionIdType funcId() const { return _funcId; }

  /**
   * @brief Get `this` object when calling the function.
   *
   * @return Value* `this` object when calling the function.
   */
  Value* GetThis() const { return _this; }

  /**
   * @brief Set `this` object when calling the function.
   *
   * @param thisValue `this` object when calling the function.
   */
  void SetThis(Value* thisValue) {
    _this = thisValue;
  }

  /**
   * @brief Determine whether `this` object has been set.
   *
   * @return true if `this` object has been set.
   * @return false if `this` object has not been set.
   */
  bool HasThis() const { return static_cast<bool>(_this); }

  /**
   * @brief Reserve the minimum capacity of the arguments array.
   *
   * @param size the minimum capacity of the arguments arra.
   */
  void ReserveArgs(size_t size) { _args.reserve(size); }

  /**
   * @brief Get the number of arguments.
   *
   * @return size_t the number of arguments.
   */
  size_t GetArgsCount() const { return _args.size(); }

  /**
   * @brief Get the argument at the given index.
   *
   * @param index the index.
   * @return Value* the argument at the given index.
   */
  Value* GetArg(size_t index) const { return _args.at(index); }

  /**
   * @brief Set the argument at the given index.
   *
   * @param index the index.
   * @param value the value to set.
   */
  void SetArg(size_t index, Value* value) {
    _args.at(index) = std::move(value);
  }

  /**
   * @brief Add a new argument to the back of the arguments array.
   *
   * @param arg the argument value.
   */
  void PushArg(Value* arg) { _args.push_back(std::move(arg)); }

  /**
   * @brief Remove the argument at the given index.
   *
   * @param index the index of the argument to remove.
   */
  void RemoveArg(size_t index) { _args.erase(std::next(_args.begin(), index)); }

  Iterator begin() { return _args.begin(); }

  Iterator end() { return _args.end(); }

  ConstIterator begin() const { return _args.begin(); }

  ConstIterator end() const { return _args.end(); }

private:
  FunctionIdType _funcId;
  Value* _this;
  std::vector<Value*> _args;
}; // class FunctionCall

} // namespace caf

#endif
