#ifndef CAF_ARRAY_VALUE_H
#define CAF_ARRAY_VALUE_H

#include "Basic/ArrayType.h"
#include "Fuzzer/Value.h"

#include <vector>

namespace caf {

/**
 * @brief Represent a value of `ArrayType`.
 *
 */
class ArrayValue : public Value {
public:
  /**
   * @brief Construct a new ArrayValue object.
   *
   * @param pool the object pool containing this value.
   * @param type type of the array value.
   * @param elements elements contained in the array.
   */
  explicit ArrayValue(CAFObjectPool* pool, const ArrayType* type, std::vector<Value *> elements)
    : Value { pool, ValueKind::ArrayValue, type },
      _size(type->size()),
      _elements(std::move(elements))
  { }

  /**
   * @brief Construct a new ArrayValue object.
   *
   * @param pool the object pool containing this value.
   * @param type type of the array value.
   */
  explicit ArrayValue(CAFObjectPool* pool, const ArrayType* type)
    : Value { pool, ValueKind::ArrayValue, type },
      _size(type->size())
  { }

  /**
   * @brief Get the number of elements in this array.
   *
   * @return size_t the number of elements in this array.
   */
  size_t size() const { return _size; }

  /**
   * @brief Set the element value at the given index.
   *
   * @param index the index of the element within the array.
   * @param value the value to be set.
   */
  void SetElement(size_t index, Value* value) { _elements[index] = value; }

  /**
   * @brief Add the given value to this array.
   *
   * @param value the value to be added.
   */
  void AddElement(Value* value) { _elements.push_back(value); }

  /**
   * @brief Get the elements contained in this array.
   *
   * @return const std::vector<Value *> & elements contained in this array.
   */
  const std::vector<Value *>& elements() const { return _elements; }

private:
  size_t _size;
  std::vector<Value *> _elements;
};

} // namespace caf

#endif
