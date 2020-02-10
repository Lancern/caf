#ifndef CAF_FUNCTION_CALL_H
#define CAF_FUNCTION_CALL_H

#include <vector>

namespace caf {

class Function;
class Value;

/**
 * @brief Provide information on function call.
 *
 */
class FunctionCall {
public:
  /**
   * @brief Construct a new FunctionCall object.
   *
   * @param func the function to be called.
   */
  explicit FunctionCall(const Function* func)
    : _func(func),
      _args { }
  { }

  /**
   * @brief Get the function called.
   *
   * @return const Function* pointer to the function definition to be called.
   */
  const Function* func() const { return _func; }

  /**
   * @brief Add an argument to the function call.
   *
   * @param arg argument to the function call.
   */
  void AddArg(Value* arg) { _args.push_back(arg); }

  /**
   * @brief Set the argument at the given index to the specified value.
   *
   * @param index the index of the argument.
   * @param value the value of the argument.
   */
  void SetArg(size_t index, Value* value) { _args[index] = value; }

  /**
   * @brief Get the arguments to the function being called.
   *
   * @return const std::vector<Value *>& arguments to the function being called.
   */
  const std::vector<Value *>& args() const { return _args; }

private:
  const Function* _func;
  std::vector<Value *> _args;
}; // class FunctionCall

} // namespace caf

#endif
