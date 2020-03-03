#include "Fuzzer/TestCaseDeserializer.h"

#include <cassert>

namespace caf {

namespace details {

size_t TestCaseDeserializationContext::AllocValueIndex() {
  auto index = _values.size();
  _values.push_back(nullptr);
  return index;
}

void TestCaseDeserializationContext::SetValue(size_t index, Value* value) {
  _values[index] = value;
}

Value* TestCaseDeserializationContext::GetValue(size_t index) const {
  return _values.at(index);
}

void TestCaseDeserializationContext::SkipNextValueIndex() {
  _values.push_back(nullptr);
}

void TestCaseDeserializationContext::SkipNextPlaceholderIndex() {
  _placeholderIndexAlloc.next();
}

void TestCaseDeserializationContext::ReservePlaceholderIndex() {
  auto plcaeholderIndex = _placeholderIndexAlloc.next();
  auto objectPoolIndex = _values.size();
  _placeholderIndexAdjustment[objectPoolIndex] = plcaeholderIndex;
}

bool TestCaseDeserializationContext::IsPlaceholderIndex(size_t objectPoolIndex) const {
  return _placeholderIndexAdjustment.find(objectPoolIndex) != _placeholderIndexAdjustment.end();
}

size_t TestCaseDeserializationContext::GetPlaceholderIndex(size_t objectPoolIndex) const {
  auto i = _placeholderIndexAdjustment.find(objectPoolIndex);
  assert(i != _placeholderIndexAdjustment.end() &&
      "No such placeholder index that is associated with the given object pool index.");
  return i->second;
}

} // namespace details

} // namespace caf
