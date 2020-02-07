#ifndef CAF_OBJECT_POOL_H
#define CAF_OBJECT_POOL_H

#include "Infrastructure/Memory.h"
#include "Infrastructure/Random.h"
#include "Fuzzer/Value.h"

#include <vector>
#include <memory>

namespace caf {

class Value;

/**
 * @brief Pool of objects that can be passed as function arguments of some specific type.
 *
 * Instances of this class cannot be copied.
 *
 */
class CAFObjectPool {
public:
  /**
   * @brief Construct a new CAFObjectPool object.
   *
   */
  explicit CAFObjectPool()
    : _values { }
  { }

  CAFObjectPool(const CAFObjectPool &) = delete;
  CAFObjectPool(CAFObjectPool &&) = default;

  CAFObjectPool& operator=(const CAFObjectPool &) = delete;
  CAFObjectPool& operator=(CAFObjectPool &&) = default;

  /**
   * @brief Get all values contained in this object pool.
   *
   * @return mapped_vector_range<const Value *, std::unique_ptr<Value>> a range of all values
   * contained in this object pool.
   */
  const std::vector<std::unique_ptr<Value>>& values() const {
    return _values;
  }

  /**
   * @brief Get the number of values contained in this object pool.
   *
   * @return size_t the number of values contained in this object pool.
   */
  size_t size() const { return static_cast<size_t>(_values.size()); }

  /**
   * @brief Determine whether the object pool is empty.
   *
   * @return true if the object pool is empty.
   * @return false if the object pool is not empty.
   */
  bool empty() const { return size() == 0; }

  /**
   * @brief Create a new value in the object pool.
   *
   * @tparam T the type of the value to be created. T should derive from `Value`.
   * @tparam Args the types of the arguments for constructing a new value.
   * @param args arguments for constructing a new value.
   * @return T* pointer to the created value.
   */
  template <typename T, typename ...Args>
  T* CreateValue(Args&&... args) {
    static_assert(std::is_base_of<Value, T>::value, "T does not derive from Value.");
    _values.push_back(caf::make_unique<T>(std::forward<Args>(args)...));
    return dynamic_cast<T *>(_values.back().get());
  }

  /**
   * @brief Randomly select a value from this @see ObjectPool, using the given random number
   * generator.
   *
   * The behavior is undefined if this @see ObjectPool is empty.
   *
   * @tparam RNG type of the random number generator. `RNG` should meets the requirements of C++
   * named requirement `RandomNumberGenerator`.
   * @param rng the random number generator.
   * @return Value* the selected value.
   */
  template <typename RNG>
  Value* Select(caf::Random<RNG>& random) const {
    return random.Select(_values)->get();
  }

private:
  std::vector<std::unique_ptr<Value>> _values;
};

} // namespace caf

#endif
