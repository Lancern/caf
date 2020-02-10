#include "Fuzzer/TestCaseSerializer.h"

namespace caf {

namespace details {

bool TestCaseSerializationContext::HasValue(Value* value) const {
  return _valueIds.find(value) != _valueIds.end();
}

size_t TestCaseSerializationContext::GetValueIndex(Value* value) const {
  return _valueIds.find(value)->second;
}

void TestCaseSerializationContext::AddValue(Value* value) {
  if (HasValue(value)) {
    return;
  }
  _valueIds[value] = _valueIdAlloc.next();
}

} // namespace details

} // namespace caf
