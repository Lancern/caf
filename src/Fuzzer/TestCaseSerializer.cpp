#include "Fuzzer/TestCaseSerializer.h"

#include <unordered_set>

namespace caf {

namespace details {

bool TestCaseSerializationContext::HasValue(const Value* value) const {
  return _valueIds.find(value) != _valueIds.end();
}

size_t TestCaseSerializationContext::GetValueIndex(const Value* value) const {
  return _valueIds.find(value)->second;
}

void TestCaseSerializationContext::AddValue(const Value* value) {
  if (HasValue(value)) {
    return;
  }
  _valueIds[value] = _valueIdAlloc.next();
}

void TestCaseSerializationContext::SkipCurrentIndex() {
  _valueIdAlloc.next();
}

void TestCaseSerializationContext::SetPlaceholderIndexAdjustment() {
  auto placeholderIndex = _placeholderIndexAlloc.next();
  auto actualIndex = _valueIdAlloc.peek();
  _adjustedPlaceholderIndexes[placeholderIndex] = actualIndex;
}

size_t TestCaseSerializationContext::GetPlaceholderIndexAdjustment(size_t placeholderIndex) const {
  auto i = _adjustedPlaceholderIndexes.find(placeholderIndex);
  assert(i != _adjustedPlaceholderIndexes.end() &&
      "The given test case placeholder index has not been registered for adjustment.");
  return i->second;
}

} // namespace details

} // namespace caf
