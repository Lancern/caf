#include "Infrastructure/Intrinsic.h"
#include "Infrastructure/Stream.h"
#include "Basic/Function.h"
#include "Fuzzer/TestCaseDeserializer.h"
#include "Fuzzer/ObjectPool.h"
#include "Fuzzer/FunctionCall.h"

#include <utility>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace caf {

namespace {

template <size_t Size, typename T>
T ReadInt(InputStream& in) {
  uint8_t data[Size];
  in.Read(data, Size);
  return static_cast<T>(
    *reinterpret_cast<typename caf::MakeIntegralType<Size, std::is_signed<T>::value>::Type *>(data)
  );
}

double ReadFloat(InputStream& in) {
  double value;
  in.Read(&value, sizeof(value));
  return value;
}

} // namespace <anonymous>

class TestCaseDeserializer::DeserializationContext {
public:
  explicit DeserializationContext() = default;

  DeserializationContext(const DeserializationContext &) = default;
  DeserializationContext(DeserializationContext &&) noexcept = default;

  DeserializationContext& operator=(const DeserializationContext &) = default;
  DeserializationContext& operator=(DeserializationContext &&) = default;

  void SetNextValue(Value* value) {
    _pool.push_back(value);
  }

  void SetNextValueAsReturnValue(size_t funcIndex) {
    size_t index = _pool.size();
    _pool.push_back(nullptr);
    _retValueIndex[index] = funcIndex;
  }

  Value* GetValue(size_t index) const {
    return _pool.at(index);
  }

  size_t GetReturnValueIndex(size_t index) const {
    return _retValueIndex.at(index);
  }

  bool IsReturnValueIndex(size_t index) const {
    return _retValueIndex.find(index) != _retValueIndex.end();
  }

private:
  std::vector<Value *> _pool;
  std::unordered_map<size_t, size_t> _retValueIndex;
}; // class TestCaseDeserializer::DeserializationContext

TestCase TestCaseDeserializer::Deserialize() {
  DeserializationContext context { };

  TestCase tc { };
  auto callsCount = ReadInt<4, size_t>(_in);
  tc.ReserveFunctionCalls(callsCount);
  for (size_t i = 0; i < callsCount; ++i) {
    auto call = DeserializeFunctionCall(context);
    tc.PushFunctionCall(std::move(call));
    context.SetNextValueAsReturnValue(i);
  }

  return tc;
}

FunctionCall TestCaseDeserializer::DeserializeFunctionCall(DeserializationContext& context) {
  auto funcId = ReadInt<4, FunctionIdType>(_in);

  FunctionCall call { funcId };
  auto thisValue = DeserializeValue(context);
  call.SetThis(thisValue);

  auto isCtor = ReadInt<1, uint8_t>(_in);
  call.SetConstructorCall(isCtor);

  auto argsCount = ReadInt<4, size_t>(_in);
  call.ReserveArgs(argsCount);

  for (size_t i = 0; i < argsCount; ++i) {
    auto arg = DeserializeValue(context);
    call.PushArg(arg);
  }

  return call;
}

Value* TestCaseDeserializer::DeserializeValue(DeserializationContext& context) {
  auto kind = static_cast<ValueKind>(ReadInt<1, uint8_t>(_in));
  switch (kind) {
    case ValueKind::Undefined:
      return _pool.GetUndefinedValue();
    case ValueKind::Null:
      return _pool.GetNullValue();
    case ValueKind::Function: {
      auto funcId = ReadInt<4, FunctionIdType>(_in);
      return _pool.GetFunctionValue(funcId);
    }
    case ValueKind::Boolean: {
      auto value = static_cast<bool>(ReadInt<1, uint8_t>(_in));
      return _pool.GetBooleanValue(value);
    }
    case ValueKind::String: {
      auto len = ReadInt<4, size_t>(_in);
      std::string s;
      s.reserve(len);
      for (size_t i = 0; i < len; ++i) {
        s.push_back(static_cast<char>(_in.ReadByte()));
      }
      return _pool.GetOrCreateStringValue(std::move(s));
    }
    case ValueKind::Integer: {
      auto value = ReadInt<4, int32_t>(_in);
      return _pool.GetOrCreateIntegerValue(value);
    }
    case ValueKind::Float: {
      auto value = ReadFloat(_in);
      return _pool.GetOrCreateFloatValue(value);
    }
    case ValueKind::Array: {
      auto arrayValue = _pool.CreateArrayValue();
      context.SetNextValue(arrayValue);
      auto size = ReadInt<4, size_t>(_in);
      arrayValue->reserve(size);
      for (size_t i = 0; i < size; ++i) {
        auto element = DeserializeValue(context);
        arrayValue->Push(element);
      }
      return arrayValue;
    }
    case ValueKind::Placeholder: {
      auto index = ReadInt<4, size_t>(_in);
      if (context.IsReturnValueIndex(index)) {
        index = context.GetReturnValueIndex(index);
        return _pool.GetPlaceholderValue(index);
      } else {
        return context.GetValue(index);
      }
    }
    default:
      CAF_UNREACHABLE;
  }
  return nullptr; // Make the compiler happy.
}

} // namespace caf
