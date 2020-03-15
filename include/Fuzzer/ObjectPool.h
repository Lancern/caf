#ifndef CAF_OBJECT_POOL_H
#define CAF_OBJECT_POOL_H

#include "Infrastructure/Memory.h"
#include "Infrastructure/Random.h"
#include "Fuzzer/Value.h"

#include <cstdint>
#include <utility>
#include <memory>
#include <vector>
#include <unordered_map>
#include <type_traits>

namespace caf {

/**
 * @brief Object pool is the owner of Value objects.
 *
 */
class ObjectPool {
public:
  /**
   * @brief Construct a new ObjectPool object.
   *
   */
  explicit ObjectPool();

  ObjectPool(const ObjectPool &) = delete;
  ObjectPool(ObjectPool &&) noexcept = default;

  ObjectPool& operator=(const ObjectPool &) = delete;
  ObjectPool& operator=(ObjectPool &&) = default;

  /**
   * @brief Create a new Value object.
   *
   * @tparam T type of the Value object.
   * @tparam Args types of arguments to construct an object of type T.
   * @param args arguments to construct an object of type T.
   * @return T* the created Value object.
   */
  template <typename T, typename ...Args>
  T* CreateValue(Args&&... args) {
    static_assert(std::is_base_of<Value, T>::value, "T does not derive from Value.");
    auto value = caf::make_unique<T>(std::forward<Args>(args)...);
    auto ret = value.get();
    _values.push_back(std::move(value));
    return ret;
  }

  /**
   * @brief Get the undefined value.
   *
   * @return Value* the undefined value.
   */
  Value* GetUndefinedValue();

  /**
   * @brief Get the null value.
   *
   * @return Value* the null value.
   */
  Value* GetNullValue();

  /**
   * @brief Get the function value.
   *
   * @return Value* the function value.
   */
  Value* GetFunctionValue();

  /**
   * @brief Get the boolean value corresponding to the given bool value.
   *
   * @param value the bool value.
   * @return BooleanValue* the boolean value.
   */
  BooleanValue* GetBooleanValue(bool value);

  /**
   * @brief Get or create a StringValue representing the given string.
   *
   * @param s the C++ string.
   * @return StringValue* the StringValue representing the given string.
   */
  StringValue* GetOrCreateStringValue(std::string s);

  /**
   * @brief Get or create an IntegerValue representing the given integer.
   *
   * @param value the integer value.
   * @return IntegerValue* the created IntegerValue object representing the given integer.
   */
  IntegerValue* GetOrCreateIntegerValue(int32_t value);

  /**
   * @brief Get or create a FloatValue representing the given floating point value.
   *
   * @param value the C++ floating point value.
   * @return FloatValue* the floating point value.
   */
  FloatValue* GetOrCreateFloatValue(double value);

  /**
   * @brief Create an ArrayValue object representing an empty array.
   *
   * @return ArrayValue* the created array value object.
   */
  ArrayValue* CreateArrayValue() {
    return CreateValue<ArrayValue>();
  }

  /**
   * @brief Get a PlaceholderValue representing the given index reference.
   *
   * @param index the reference.
   * @return PlaceholderValue* the placeholder value.
   */
  PlaceholderValue* GetPlaceholderValue(size_t index);

  /**
   * @brief Determine whether this object pool is empty.
   *
   * @return true if this object pool is empty.
   * @return false if this object pool is not empty.
   */
  bool empty() const { return _values.empty(); }

  /**
   * @brief Get the number of values managed by this ObjectPool object.
   *
   * @return size_t the number of values managed by this ObjectPool object.
   */
  size_t GetValuesCount() const { return _values.size(); }

  /**
   * @brief Get the Value object at the given index.
   *
   * @param index the index.
   * @return Value* the value at the given index.
   */
  Value* GetValue(size_t index) const { return _values.at(index).get(); }

  /**
   * @brief Randomly select a value from this object pool, using the given random number generator.
   *
   * @tparam T type of the random bits generator used by the random number generator.
   * @param rnd the random number generator.
   * @return Value* the selected value.
   */
  template <typename T>
  Value* SelectValue(Random<T>& rnd) {
    return rnd.Select(_values).get();
  }

private:
  std::vector<std::unique_ptr<Value>> _values;
  std::unique_ptr<Value> _undef; // Undefined value
  std::unique_ptr<Value> _null; // Null value
  std::unique_ptr<Value> _func; // Function value
  std::unique_ptr<BooleanValue> _bool[2]; // Boolean values
  std::unordered_map<std::string, StringValue *> _strToValue;
  std::unique_ptr<IntegerValue *[]> _intTable;
  std::unique_ptr<FloatValue> _nan; // NaN value.
  std::unique_ptr<FloatValue> _inf; // +infinity value.
  std::unique_ptr<FloatValue> _negInf; // -infinity value.
  std::vector<std::unique_ptr<PlaceholderValue>> _placeholderValues;
}; // class ObjectPool

} // namespace caf

#endif
