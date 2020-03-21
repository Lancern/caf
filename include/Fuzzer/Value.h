#ifndef CAF_VALUE_H
#define CAF_VALUE_H

#include "Infrastructure/Casting.h"
#include "Basic/Function.h"

#include <cstdint>
#include <utility>
#include <memory>
#include <string>
#include <vector>

namespace caf {

#define CAF_VALUE_KIND_LIST(V) \
  V(Undefined) \
  V(Null) \
  V(Boolean) \
  V(String) \
  V(Function) \
  V(Integer) \
  V(Float) \
  V(Array) \
  V(Placeholder)

/**
 * @brief Kinds of a language specific value.
 *
 */
enum class ValueKind : uint8_t {
#define DECL_ENUMERATOR(name) name,
  CAF_VALUE_KIND_LIST(DECL_ENUMERATOR)
#undef DECL_ENUMERATOR
}; // enum class ValueKind

/**
 * @brief Represent a language specific value.
 *
 */
class Value {
public:
  /**
   * @brief Construct a new Value object.
   *
   * @param kind kind of this value.
   */
  explicit Value(ValueKind kind)
    : _kind(kind)
  { }

  virtual ~Value() = default;

  /**
   * @brief Get the kind of this value.
   *
   * @return ValueKind kind of this value.
   */
  ValueKind kind() const { return _kind; }

#define DECL_TYPE_CHECK_METHOD(k) \
    bool Is##k() const { return _kind == ValueKind::k; }
  CAF_VALUE_KIND_LIST(DECL_TYPE_CHECK_METHOD)
#undef DECL_TYPE_CHECK_METHOD

  /**
   * @brief Get the bool value represented by this value.
   *
   * This function will trigger an assertion failure if this value is not a boolean value.
   *
   * @return bool the boolean value represented by this value.
   */
  virtual bool GetBooleanValue() const {
    assert(false && "The current value is not a boolean value.");
  }

  /**
   * @brief Get the integer value represented by this value.
   *
   * This function will trigger an assertion failure if this value is not an integer value.
   *
   * @return int32_t the integer value represented by this value.
   */
  virtual int32_t GetIntegerValue() const {
    assert(false && "The current value is not an integer value.");
  }

  /**
   * @brief Get the floating point value represented by this value.
   *
   * This function will trigger an assertion failure if this value is not a floating point value.
   *
   * @return double the boolean value represented by this value.
   */
  virtual double GetFloatValue() const {
    assert(false && "The current value is not a floating point value.");
  }

  /**
   * @brief Get the string value represented by this value.
   *
   * This function will trigger an assertion failure if this value is not a string value.
   *
   * @return const std::string& the string value represented by this value.
   */
  virtual const std::string& GetStringValue() const {
    assert(false && "The current value is not a string value.");
  }

  /**
   * @brief Get the function ID that this function value reference to.
   *
   * This function will trigger an assertion failure if this value is not a function value.
   *
   * @return FunctionIdType the function ID.
   */
  virtual FunctionIdType GetFunctionId() const {
    assert(false && "The current value is not a function value.");
  }

  /**
   * @brief Get the reference index of this placeholder value.
   *
   * This function will trigger an assertion failure if this value is not a placeholder value.
   *
   * @return size_t the reference index represented by this value.
   */
  virtual size_t GetPlaceholderIndex() const {
    assert(false && "The current value is not a placeholder value.");
  }

  /**
   * @brief Create a new Value object representing an undefined value.
   *
   * @return Value the value created.
   */
  static Value CreateUndefinedValue() { return Value { ValueKind::Undefined }; }

private:
  ValueKind _kind;
}; // class Value

/**
 * @brief A language specific boolean value.
 *
 */
class BooleanValue : public Value {
public:
  /**
   * @brief Construct a new BooleanValue object.
   *
   * @param value the boolean value.
   */
  explicit BooleanValue(bool value)
    : Value { ValueKind::Boolean },
      _value(value)
  { }

  /**
   * @brief Get the boolean value.
   *
   * @return bool the boolean value.
   */
  bool value() const { return _value; }

  bool GetBooleanValue() const override {
    return value();
  }

private:
  bool _value;
}; // class BooleanValue

/**
 * @brief A language specific string value.
 *
 */
class StringValue : public Value {
public:
  /**
   * @brief Construct a new StringValue object.
   *
   * @param str the string value.
   */
  explicit StringValue(std::string str)
    : Value { ValueKind::String },
      _value(std::move(str))
  { }

  /**
   * @brief Get the string value.
   *
   * @return const std::string& the string value.
   */
  const std::string& value() const { return _value; }

  /**
   * @brief Get the length of the string.
   *
   * @return size_t length of the string.
   */
  size_t length() const { return _value.length(); }

  const std::string& GetStringValue() const override {
    return value();
  }

private:
  std::string _value;
}; // class StringValue

/**
 * @brief Represent a function value.
 *
 */
class FunctionValue : public Value {
public:
  /**
   * @brief Construct a new FunctionValue object.
   *
   * @param funcId the function ID.
   */
  explicit FunctionValue(FunctionIdType funcId)
    : Value { ValueKind::Function },
      _funcId(funcId)
  { }

  /**
   * @brief Get the function ID.
   *
   * @return FunctionIdType the function ID.
   */
  FunctionIdType funcId() const { return _funcId; }

  FunctionIdType GetFunctionId() const override {
    return _funcId;
  }

private:
  FunctionIdType _funcId;
}; // class FunctionValue

/**
 * @brief A language specific integer value.
 *
 */
class IntegerValue : public Value {
public:
  constexpr static const size_t BitLength = 32;

  /**
   * @brief Construct a new IntegerValue object.
   *
   * @param value the integer value.
   */
  explicit IntegerValue(int32_t value)
    : Value { ValueKind::Integer },
      _value(value)
  { }

  /**
   * @brief Get the integer value.
   *
   * @return int32_t the integer value.
   */
  int32_t value() const { return _value; }

  int32_t GetIntegerValue() const override {
    return value();
  }

private:
  int32_t _value;
}; // class IntegerValue

/**
 * @brief A language specific floating point value.
 *
 */
class FloatValue : public Value {
public:
  /**
   * @brief Construct a new FloatValue object.
   *
   * @param value the value.
   */
  explicit FloatValue(double value)
    : Value { ValueKind::Float },
      _value(value)
  { }

  /**
   * @brief Get the floating point value.
   *
   * @return double the floating point value.
   */
  double value() const { return _value; }

  double GetFloatValue() const override {
    return value();
  }

private:
  double _value;
}; // class FloatValue

/**
 * @brief A language specific array value.
 *
 */
class ArrayValue : public Value {
public:
  using Iterator = typename std::vector<Value *>::iterator;
  using ConstIterator = typename std::vector<Value *>::const_iterator;

  /**
   * @brief Construct a new ArrayValue object.
   *
   */
  explicit ArrayValue()
    : Value { ValueKind::Array }
  { }

  /**
   * @brief Get the number of elements in this array.
   *
   * @return size_t the number of elements in this array.
   */
  size_t size() const { return _elements.size(); }

  /**
   * @brief Reserve a capacity of at least size elements.
   *
   * @param size the minimum capacity.
   */
  void reserve(size_t size) {
    _elements.reserve(size);
  }

  /**
   * @brief Add a value to the back of the array.
   *
   * @param value the value to be added.
   */
  void Push(Value* value) {
    _elements.push_back(std::move(value));
  }

  /**
   * @brief Get the element at the given index.
   *
   * @param index the index.
   * @return Value* the element at the given index.
   */
  Value* GetElement(size_t index) const { return _elements.at(index); }

  /**
   * @brief Set the element at the given index.
   *
   * @param index the index.
   * @param value the new value.
   */
  void SetElement(size_t index, Value* value) {
    _elements.at(index) = std::move(value);
  }

  const Value* operator[](size_t index) const { return _elements.at(index); }

  Iterator begin() { return _elements.begin(); }

  Iterator end() { return _elements.end(); }

  ConstIterator begin() const { return _elements.begin(); }

  ConstIterator end() const { return _elements.end(); }

private:
  std::vector<Value*> _elements;
};

/**
 * @brief A placeholder value is not a language specific value. Instead it is used by CAF to
 * reference the return value of some previous function call.
 *
 */
class PlaceholderValue : public Value {
public:
  /**
   * @brief Construct a new PlaceholderValue object.
   *
   * @param index the index of the function call.
   */
  explicit PlaceholderValue(size_t index)
    : Value { ValueKind::Placeholder },
      _index(index)
  { }

  /**
   * @brief Get the index of the function call.
   *
   * @return size_t index of the function call.
   */
  size_t index() const { return _index; }

  size_t GetPlaceholderIndex() const override {
    return _index;
  }

private:
  size_t _index;
}; // class PlaceholderValue

} // namespace caf

#endif
