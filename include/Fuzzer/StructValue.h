#ifndef CAF_STRUCT_VALUE_H
#define CAF_STRUCT_VALUE_H

#include "Basic/StructType.h"
#include "Basic/Constructor.h"
#include "Fuzzer/Value.h"

namespace caf {

/**
 * @brief Represent a value of `StructType`.
 *
 */
class StructValue : public Value {
public:
  /**
   * @brief Construct a new StructValue object.
   *
   * @param pool the object pool containing this value.
   * @param type the type of the value.
   * @param constructor the constructor to use to activate the object.
   * @param args arguments passed to the object.
   */
  explicit StructValue(CAFObjectPool* pool, const StructType* type,
      const Constructor* constructor, std::vector<Value *> args)
    : StructValue { pool, type, constructor }
  {
    _args = std::move(args);
  }

  /**
   * @brief Construct a new StructValue object.
   *
   * @param pool the object pool containing this value.
   * @param type the type of the value.
   * @param constructor the constructor to use to activate the object.
   */
  explicit StructValue(CAFObjectPool* pool, const StructType* type, const Constructor* constructor)
    : Value { pool, ValueKind::StructValue, type },
      _constructor(constructor),
      _args { }
  { }

  /**
   * @brief Get the constructor used to activate objects.
   *
   * @return const Constructor* constructor used to activate objects.
   */
  const Constructor* ctor() const { return _constructor; }

  /**
   * @brief Set the argument value at the given index.
   *
   * @param index the index of the argument.
   * @param value the value of the argument to set.
   */
  void SetArg(size_t index, Value* value) { _args[index] = value; }

  /**
   * @brief Add a new argument to the constructor argument list.
   *
   * @param value the value to be added.
   */
  void AddArg(Value* value) { _args.push_back(value); }

  /**
   * @brief Get the arguments passed to the activator.
   *
   * @return const std::vector<Value *> & list of arguments passed to the activator.
   */
  const std::vector<Value *>& args() const { return _args; }

  /**
   * @brief Get the number of arguments passed to the constructor.
   *
   * @return size_t the number of arguments passed to the constructor.
   */
  size_t GetArgsCount() const { return _args.size(); }

  /**
   * @brief Get the constructor argument at the given index.
   *
   * @param index the index of the constructor argument.
   * @return Value* the value of the constructor argument.
   */
  Value* GetArg(size_t index) const { return _args[index]; }

private:
  const Constructor *_constructor;
  std::vector<Value *> _args;
};

} // namespace caf

#endif
