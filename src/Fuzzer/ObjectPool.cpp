#include "Infrastructure/Memory.h"
#include "Fuzzer/ObjectPool.h"

#include <cmath>
#include <limits>
#include <utility>
#include <algorithm>

namespace caf {

namespace {

template <typename T, typename ...Args>
T* get_or_create(std::unique_ptr<T>& ptr, Args&&... args) {
  if (!ptr) {
    ptr = caf::make_unique<T>(std::forward<Args>(args)...);
  }
  return ptr.get();
}

} // namespace <anonymous>

constexpr const static size_t INTEGER_TABLE_SIZE = 500;
constexpr const static int32_t INTEGER_BIAS = 100;
constexpr const static size_t MAX_STRING_LEN_IN_TABLE = 10;
constexpr const static size_t PLACEHOLDER_TABLE_INIT_SIZE = 10;

ObjectPool::ObjectPool()
  : _values(),
    _undef(nullptr),
    _null(nullptr),
    _funcValues(),
    _bool { nullptr, nullptr },
    _strToValue(),
    _intTable(caf::make_unique<IntegerValue* []>(INTEGER_TABLE_SIZE)),
    _nan(nullptr),
    _inf(nullptr),
    _negInf(nullptr),
    _arrayValues(),
    _placeholderValues(PLACEHOLDER_TABLE_INIT_SIZE)
{ }

Value* ObjectPool::GetUndefinedValue() {
  return get_or_create(_undef, ValueKind::Undefined);
}

Value* ObjectPool::GetNullValue() {
  return get_or_create(_null, ValueKind::Null);
}

FunctionValue* ObjectPool::GetFunctionValue(FunctionIdType funcId) {
  auto i = _funcValues.find(funcId);
  if (i == _funcValues.end()) {
    return _funcValues.emplace(funcId, caf::make_unique<FunctionValue>(funcId)).first->second.get();
  } else {
    return i->second.get();
  }
}

BooleanValue* ObjectPool::GetBooleanValue(bool value) {
  return get_or_create(_bool[static_cast<int>(value)], value);
}

StringValue* ObjectPool::GetOrCreateStringValue(std::string s) {
  if (s.length() <= MAX_STRING_LEN_IN_TABLE) {
    auto i = _strToValue.find(s);
    if (i != _strToValue.end()) {
      return i->second;
    }

    auto value = CreateValue<StringValue>(s);
    _strToValue.emplace(std::move(s), value);
    return value;
  } else {
    return CreateValue<StringValue>(std::move(s));
  }
}

IntegerValue* ObjectPool::GetOrCreateIntegerValue(int32_t value) {
  bool inTable = false;
  size_t index = 0;
  if (value <= std::numeric_limits<int32_t>::max() - INTEGER_BIAS) {
    index = value + INTEGER_BIAS;
    if (index >= 0 && index < INTEGER_TABLE_SIZE) {
      inTable = true;
    }
  }

  if (inTable && _intTable[index]) {
    return _intTable[index];
  }

  auto intValue = CreateValue<IntegerValue>(value);
  if (inTable) {
    _intTable[index] = intValue;
  }
  return intValue;
}

FloatValue* ObjectPool::GetOrCreateFloatValue(double value) {
  switch (std::fpclassify(value)) {
    case FP_NAN: {
      if (!_nan) {
        _nan = caf::make_unique<FloatValue>(std::numeric_limits<double>::quiet_NaN());
      }
      return _nan.get();
    }
    case FP_INFINITE: {
      if (value > 0) {
        // value is positive infinity.
        return get_or_create(_inf, std::numeric_limits<double>::infinity());
      } else {
        // value is negative infinity.
        return get_or_create(_inf, -std::numeric_limits<double>::infinity());
      }
    }
    default:
      return CreateValue<FloatValue>(value);
  }
}

ArrayValue* ObjectPool::CreateArrayValue() {
  auto value = caf::make_unique<ArrayValue>();
  auto ret = value.get();
  _arrayValues.push_back(std::move(value));
  return ret;
}

PlaceholderValue* ObjectPool::GetPlaceholderValue(size_t index) {
  if (index >= _placeholderValues.size()) {
    _placeholderValues.resize(index + 1);
  }

  auto& ptr = _placeholderValues.at(index);
  if (!ptr) {
    ptr = caf::make_unique<PlaceholderValue>(index);
  }
  return ptr.get();
}

void ObjectPool::clear() {
  _values.clear();
  _strToValue.clear();
  std::fill(_intTable.get(), _intTable.get() + INTEGER_TABLE_SIZE, nullptr);
}

} // namespace caf
