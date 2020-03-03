#ifndef CAF_FUNCTION_POINTER_VALUE_H
#define CAF_FUNCTION_POINTER_VALUE_H

#include "Basic/FunctionType.h"
#include "Fuzzer/Value.h"

#include <cstdint>

namespace caf {

/**
 * @brief Represent a value of a function pointer.
 *
 */
class FunctionValue : public Value {
public:
  /**
   * @brief Construct a new FunctionValue object.
   *
   * @param pool the object pool containing this value.
   * @param functionId the ID of the function.
   * @param type the type of the function.
   */
  explicit FunctionValue(CAFObjectPool* pool, uint64_t functionId, const FunctionType* type)
    : Value { pool, ValueKind::FunctionValue, type },
      _functionId(functionId)
  { }

  /**
   * @brief Get the ID of the function.
   *
   * @return uint64_t ID of the function.
   */
  uint64_t functionId() const { return _functionId; }

private:
  uint64_t _functionId;
}; // class FunctionPointerValue

} // namespace caf

#endif
