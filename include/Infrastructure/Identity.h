#ifndef CAF_IDENTITY_H
#define CAF_IDENTITY_H

#include "json/json.hpp"

#include <type_traits>

namespace caf {

/**
 * @brief Provide an ID allocator to allocate self-increment IDs. This class is not thread-safe.
 * Multiple instances of IdAllocator are independent, and they may produce the same IDs.
 *
 */
template <typename T>
class IncrementIdAllocator {
  static_assert(std::is_integral<T>::value, "T should be an integral type.");

public:
  /**
   * @brief Generate next ID in ID sequence.
   *
   * @return T the next ID.
   */
  T operator()() {
    return next();
  }

  /**
   * @brief Generate next ID in ID sequence.
   *
   * @return T the next ID.
   */
  T next() {
    return _id++;
  }

  /**
   * @brief Get the next available ID but do not change the state of this ID allocator.
   *
   * @return T the next ID.
   */
  T peek() const {
    return _id;
  }

  /**
   * @brief Reset this ID allocator to its initial state.
   *
   */
  void reset() {
    _id = 0;
  }

private:
  T _id;
}; // class IdAllocator

} // namespace caf

#endif
