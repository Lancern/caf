#ifndef CAF_V8_ARRAY_BUILDER_H
#define CAF_V8_ARRAY_BUILDER_H

#include "Targets/V8/V8Traits.h"

#include <cassert>

namespace caf {

/**
 * @brief Build V8 array values.
 *
 */
class V8ArrayBuilder {
public:
  using ValueType = typename V8Traits::ValueType;
  using ArrayType = typename V8Traits::ArrayType;

  /**
   * @brief Construct a new V8ArrayBuilder object.
   *
   * @param isolate the isolate instance.
   * @param size the size of the array.
   */
  explicit V8ArrayBuilder(v8::Isolate* isolate, size_t size)
    : _arr(v8::Array::New(isolate, size)),
      _len(size),
      _nextIndex(0)
  { }

  V8ArrayBuilder(const V8ArrayBuilder &) = delete;
  V8ArrayBuilder(V8ArrayBuilder &&) noexcept = default;

  V8ArrayBuilder& operator=(const V8ArrayBuilder &) = delete;
  V8ArrayBuilder& operator=(V8ArrayBuilder &&) = default;

  /**
   * @brief Add a new element to the array.
   *
   * @param element the element to add.
   */
  void PushElement(ValueType element) {
    assert(_nextIndex < _len && "Too many elements.");
    _arr->Set(_nextIndex++, element);
  }

  /**
   * @brief Get the array value under construction.
   *
   * @return ArrayType the array value under construction.
   */
  ArrayType GetValue() const { return _arr; }

private:
  ArrayType _arr;
  size_t _len;
  size_t _nextIndex;
};

} // namespace caf

#endif
