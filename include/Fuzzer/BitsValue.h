#ifndef CAF_BITS_VALUE_H
#define CAF_BITS_VALUE_H

#include "Basic/BitsType.h"
#include "Fuzzer/Value.h"

#include <vector>

namespace caf {

/**
 * @brief Represent a value of `BitsType`.
 *
 */
class BitsValue : public Value {
public:
  /**
   * @brief Construct a new BitsValue object. The data of the value will be initialized to zeros.
   *
   * @param pool object pool containing this value.
   * @param type the type of the value.
   */
  explicit BitsValue(CAFObjectPool* pool, const BitsType* type)
    : Value { pool, ValueKind::BitsValue, type },
      _size(type->size()),
      _data { } {
    _data.resize(_size);
  }

  /**
   * @brief Get the size of the value, in bytes.
   *
   * @return size_t size of the value, in bytes.
   */
  size_t size() const { return _size; }

  /**
   * @brief Get a pointer to the buffer containing raw binary data of the value.
   *
   * @return uint8_t* a pointer to the buffer containing raw binary data of the value.
   */
  uint8_t* data() { return _data.data(); }

  /**
   * @brief Get a pointer to the buffer containing raw binary data of the value. The returned
   * pointer is immutable.
   *
   * @return const uint8_t* immutable pointer to the buffer containing raw binary data of the value.
   */
  const uint8_t* data() const { return _data.data(); }

private:
  size_t _size;
  std::vector<uint8_t> _data;
};

} // namespace caf

#endif
