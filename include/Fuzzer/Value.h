#ifndef CAF_VALUE_H
#define CAF_VALUE_H

namespace caf {

class CAFObjectPool;
class Type;

/**
 * @brief Represent the kind of a value.
 *
 */
enum class ValueKind : int {
  /**
   * @brief The value is an instance of `BitsType`.
   *
   */
  BitsValue,

  /**
   * @brief The value is an instance of `PointerType`.
   *
   */
  PointerValue,

  /**
   * @brief The value is an instance of `PointerType` and the pointee is a function.
   *
   */
  FunctionPointerValue,

  /**
   * @brief The value is an instance of `ArrayType`.
   *
   */
  ArrayValue,

  /**
   * @brief The value is an instance of `StructType`.
   *
   */
  StructValue
};

/**
 * @brief Abstract base class representing a value that can be passed to a function.
 *
 */
class Value {
public:
/**
 * @brief Destroy the Value object.
 *
 */
  virtual ~Value() = default;

  /**
   * @brief Get the object pool containing this value.
   *
   * @return CAFObjectPool* object pool containing this value.
   */
  CAFObjectPool* pool() const { return _pool; }

  /**
   * @brief Kind of the value.
   *
   * @return ValueKind kind of the value.
   */
  ValueKind kind() const { return _kind; }

  /**
   * @brief Get the type of the value.
   *
   * @return const Type* type of the value.
   */
  const Type* type() const { return _type; }

protected:
  /**
   * @brief Construct a new Value object.
   *
   * @param pool object pool containing this value.
   * @param kind kind of the value.
   * @param type the type of the value.
   */
  explicit Value(CAFObjectPool* pool, ValueKind kind, const Type* type)
    : _pool(pool),
      _kind(kind),
      _type(type)
  { }

private:
  CAFObjectPool* _pool;
  ValueKind _kind;
  const Type* _type;
};

} // namespace caf

#endif
