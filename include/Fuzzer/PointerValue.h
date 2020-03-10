#ifndef CAF_POINTER_VALUE_H
#define CAF_POINTER_VALUE_H

#include "Basic/PointerType.h"
#include "Fuzzer/Value.h"

namespace caf {

/**
 * @brief Represent a value of `PointerType`.
 *
 */
class PointerValue : public Value {
public:
  /**
   * @brief Construct a new PointerValue object.
   *
   * @param pool the object pool containing this value.
   * @param pointee value at the pointee's site.
   * @param type type of the pointer.
   */
  explicit PointerValue(CAFObjectPool* pool, Value* pointee, const PointerType* type)
    : Value { pool, ValueKind::PointerValue, type },
      _pointee(pointee)
  { }

  /**
   * @brief Construct a new PointerValue object.
   *
   * @param pool the object pool containing this value.
   * @param type the type of the pointer.
   */
  explicit PointerValue(CAFObjectPool* pool, const PointerType* type)
    : Value { pool, ValueKind::PointerValue, type },
      _pointee(nullptr)
  { }

  /**
   * @brief Determine whether this pointer value is null.
   *
   * @return true if this pointer value is null.
   * @return false if this pointer value is not null.
   */
  bool IsNull() const { return _pointee == nullptr; }

  /**
   * @brief Get the value at the pointee's site.
   *
   * @return Value* value at the pointee's site.
   */
  Value* pointee() const { return _pointee; }

  /**
   * @brief Set the pointee value.
   *
   * @param pointee the pointee value.
   */
  void SetPointee(Value* pointee) { _pointee = pointee; }

private:
  Value* _pointee;
};

} // namespace caf

#endif
