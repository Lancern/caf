#ifndef CAF_FUNCTION_POINTER_VALUE_H
#define CAF_FUNCTION_POINTER_VALUE_H

#include "Basic/PointerType.h"
#include "Fuzzer/Value.h"

#include <cstddef>

namespace caf {

/**
 * @brief Represent a value of a function pointer.
 *
 */
class FunctionPointerValue : public Value {
public:
  /**
   * @brief Construct a new FunctionPointerValue object.
   *
   * @param pool the object pool containing this value.
   * @param functionId the ID of the pointee function.
   * @param type the type of the function pointer.
   */
  explicit FunctionPointerValue(CAFObjectPool* pool, size_t functionId, const PointerType* type)
    : Value { pool, ValueKind::FunctionPointerValue, type },
      _functionId(functionId)
  { }

  /**
   * @brief Get the ID of the pointee function.
   *
   * @return size_t ID of the pointee function.
   */
  size_t functionId() const { return _functionId; }

private:
  size_t _functionId;
}; // class FunctionPointerValue

} // namespace caf

#endif
