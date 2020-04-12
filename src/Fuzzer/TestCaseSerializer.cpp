#include "Infrastructure/Casting.h"
#include "Infrastructure/Identity.h"
#include "Infrastructure/Intrinsic.h"
#include "Infrastructure/Stream.h"
#include "Fuzzer/TestCaseSerializer.h"
#include "Fuzzer/TestCase.h"
#include "Fuzzer/Value.h"

#include <cassert>
#include <unordered_map>
#include <type_traits>

namespace caf {

namespace {

template <size_t Size, typename T>
void WriteInt(OutputStream& out, T value) {
  static_assert(std::is_integral<T>::value, "T is not an integral type.");
  auto actual = caf::int_cast<Size>(value);
  out.Write(&actual, sizeof(actual));
}

void WriteFloat(OutputStream& out, double value) {
  out.Write(&value, sizeof(value));
}

} // namespace <anonymous>

class TestCaseSerializer::SerializationContext {
public:
  explicit SerializationContext() = default;

  SerializationContext(const SerializationContext &) = delete;
  SerializationContext(SerializationContext &&) noexcept = default;

  SerializationContext& operator=(const SerializationContext &) = delete;
  SerializationContext& operator=(SerializationContext &&) = default;

  bool HasValue(const Value* value) const {
    return _valueIndexes.find(value) != _valueIndexes.end();
  }

  size_t GetValueIndex(const Value* value) const {
    return _valueIndexes.at(value);
  }

  void SetNextValue(const Value* value) {
    _valueIndexes[value] = _indexAlloc.next();
  }

  void SkipNextValue() {
    _indexAlloc.next();
  }

  void SetNextValueAsReturnValue(size_t funcIndex) {
    _retValueIndexes[funcIndex] = _indexAlloc.next();
  }

  size_t GetReturnValueIndex(size_t funcIndex) {
    return _retValueIndexes.at(funcIndex);
  }

private:
  std::unordered_map<const Value *, size_t> _valueIndexes;
  std::unordered_map<size_t, size_t> _retValueIndexes;
  IncrementIdAllocator<size_t> _indexAlloc;
}; // class TestCaseSerializer::SerializationContext

void TestCaseSerializer::Serialize(const TestCase& testCase) {
  SerializationContext context { };

  WriteInt<4>(_out, testCase.storeRootEntryIndex());
  WriteInt<4>(_out, testCase.GetFunctionCallsCount());
  for (size_t i = 0; i < testCase.GetFunctionCallsCount(); ++i) {
    const auto& call = testCase.GetFunctionCall(i);
    Serialize(call, context);
    context.SetNextValueAsReturnValue(i);
  }
}

void TestCaseSerializer::Serialize(const FunctionCall& call, SerializationContext& context) {
  WriteInt<4>(_out, call.funcId());

  if (call.HasThis()) {
    Serialize(call.GetThis(), context);
  } else {
    auto undefined = Value::CreateUndefinedValue();
    Serialize(&undefined, context);
  }

  WriteInt<1>(_out, static_cast<uint8_t>(call.IsConstructorCall()));

  WriteInt<4>(_out, call.GetArgsCount());
  for (auto arg : call) {
    Serialize(arg, context);
  }
}

void TestCaseSerializer::Serialize(const Value* value, SerializationContext& context) {
  assert(value && "value cannot be null.");

  PlaceholderValue pv { 0 };
  if (value->IsPlaceholder()) {
    pv = PlaceholderValue(context.GetReturnValueIndex(value->GetPlaceholderIndex()));
    value = &pv;
  }

  if (context.HasValue(value)) {
    pv = PlaceholderValue(context.GetValueIndex(value));
    value = &pv;
  }

  if (value->IsArray()) {
    context.SetNextValue(value);
  }

  WriteInt<1>(_out, static_cast<uint8_t>(value->kind()));
  switch (value->kind()) {
    case ValueKind::Undefined:
    case ValueKind::Null:
      break;
    case ValueKind::Function:
      WriteInt<4>(_out, value->GetFunctionId());
      break;
    case ValueKind::Boolean:
      WriteInt<1>(_out, static_cast<uint8_t>(value->GetBooleanValue()));
      break;
    case ValueKind::String: {
      const auto& str = value->GetStringValue();
      WriteInt<4>(_out, str.length());
      _out.Write(str.data(), str.length());
      break;
    }
    case ValueKind::Integer:
      WriteInt<4>(_out, value->GetIntegerValue());
      break;
    case ValueKind::Float:
      WriteFloat(_out, value->GetFloatValue());
      break;
    case ValueKind::Array: {
      auto arrayValue = caf::dyn_cast<ArrayValue>(value);
      WriteInt<4>(_out, arrayValue->size());
      for (auto element : *arrayValue) {
        Serialize(element, context);
      }
      break;
    }
    case ValueKind::Placeholder:
      WriteInt<4>(_out, value->GetPlaceholderIndex());
      break;
    default:
      CAF_UNREACHABLE;
  }
}

} // namespace caf
