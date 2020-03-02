#ifndef CAF_AGGREGATE_VALUE_H
#define CAF_AGGREGATE_VALUE_H

#include "Basic/AggregateType.h"
#include "Fuzzer/Value.h"

#include <vector>

namespace caf {

class CAFObjectPool;

/**
 * @brief Represent a value of `AggregateType`.
 *
 */
class AggregateValue : public Value {
public:
  /**
   * @brief Construct a new AggregateValue object.
   *
   * @param pool the object pool containing this aggregate value.
   * @param type the type of this value.
   */
  explicit AggregateValue(CAFObjectPool* pool, const AggregateType* type)
    : Value { pool, ValueKind::AggregateValue, type }
  { }

  /**
   * @brief Construct a new AggregateValue object.
   *
   * @param pool the object pool containing this aggregate value.
   * @param type the type of this value.
   * @param fields the fields of this value.
   */
  explicit AggregateValue(CAFObjectPool* pool, const AggregateType* type, std::vector<Value *> fields)
    : Value { pool, ValueKind::AggregateValue, type },
      _fields(std::move(fields))
  { }

  /**
   * @brief Get the values of the fields.
   *
   * @return const std::vector<Value *>& the value of the fields.
   */
  const std::vector<Value *>& fields() const { return _fields; }

  /**
   * @brief Get the number of fields.
   *
   * @return size_t the number of fields.
   */
  size_t GetFieldsCount() const { return _fields.size(); }

  /**
   * @brief Get the value of the field at the given index.
   *
   * @param index the index of the field.
   * @return Value* the value of the field specified.
   */
  Value* GetField(size_t index) const { return _fields.at(index); }

  /**
   * @brief Add a new field value to this aggregate value.
   *
   * @param value the value of the new field.
   */
  void AddField(Value* value) { _fields.push_back(value); }

private:
  std::vector<Value *> _fields;
}; // class AggregateValue

} // namespace caf

#endif
