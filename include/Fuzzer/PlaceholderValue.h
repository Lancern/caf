#ifndef CAF_PLACEHOLDER_VALUE_H
#define CAF_PLACEHOLDER_VALUE_H

#include "Fuzzer/Value.h"

#include <cstddef>

namespace caf {

class Type;
class CAFObjectPool;

/**
 * @brief Placeholder values.
 *
 * A placeholder value is a special value that acts like a placeholder. Instances of
 * PlaceholderValue references some other value used in the same test case through a integer index.
 *
 * Instances of PlaceholderValue may or may not maintained by object pools. They can be either used
 * temporarily or maintained in some CAFObjectPool object.
 *
 */
class PlaceholderValue : public Value {
public:
  /**
   * @brief Construct a new Placeholder Value object. The constructed PlaceholderValue object does
   * not belong to any object pools.
   *
   * @param type the type of the value referenced by this PlaceholderValue.
   * @param valueIndex the index of the referenced value in the same test case.
   */
  explicit PlaceholderValue(const Type* type, size_t valueIndex)
    : PlaceholderValue { nullptr, type, valueIndex }
  { }

  /**
   * @brief Construct a new Placeholder Value object.
   *
   * @param pool the object pool owning this PlaceholderValue.
   * @param type the type of the value referenced by this PlaceholderValue.
   * @param valueIndex the index of the referenced value in the same test case.
   */
  explicit PlaceholderValue(CAFObjectPool* pool, const Type* type, size_t valueIndex)
    : Value { pool, ValueKind::PlaceholderValue, type },
      _index(valueIndex)
  { }

  /**
   * @brief Get the index of the referenced Value object.
   *
   * @return size_t the index of the referenced Value object.
   */
  size_t valueIndex() const { return _index; }

private:
  size_t _index;
}; // class PlaceholderValue

}; // namespace caf

#endif
