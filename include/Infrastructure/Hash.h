#ifndef CAF_HASH_H
#define CAF_HASH_H

#include <cstddef>
#include <functional>
#include <iterator>

namespace caf {

/**
 * @brief Provide a base template for hashers.
 *
 * @tparam T the type of the object on which the hasher produces hash values.
 */
template <typename T>
struct Hasher {
  size_t operator()(const T& object) const {
    std::hash<T> stdHasher { };
    return stdHasher(object);
  }
};

/**
 * @brief Combine two hash values into a single one.
 *
 * @param lhs the first hash value.
 * @param rhs the second hash value.
 * @return size_t the combined hash value.
 */
size_t CombineHash(size_t lhs, size_t rhs) {
  // Use the same algorithm used in boost's `hash_combine` to combine two hash values.
  lhs ^= rhs + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
  return lhs;
}

/**
 * @brief Get the given object's hash code, using a @see Hasher<T> template specialization.
 *
 * A @see Hasher<T> template specialization must be available when this function template is
 * instantiated.
 *
 * @tparam T the type of the object.
 * @param object the object.
 * @return size_t the hash value of the object.
 */
template <typename T>
size_t GetHashCode(const T& object) {
  Hasher<T> hasher;
  return hasher(object);
}

/**
 * @brief Get the overall hash code of a range of objects.
 *
 * @tparam Iter type of the iterator iterating over the range.
 * @param first the first iterator.
 * @param last the last iterator.
 * @return size_t the overall hash code of the given range of objects.
 */
template <typename Iter>
size_t GetRangeHashCode(Iter first, Iter last) {
  if (first == last) {
    return 0;
  }

  auto hash = GetHashCode(*first++);
  while (first != last) {
    hash = CombineHash(hash, GetHashCode(*first++));
  }

  return hash;
}

/**
 * @brief Get the overall hash code of all objects contained in the given container.
 *
 * @tparam Container the type of the container.
 * @param container the container object.
 * @return size_t the overall hash code of all objects contained in the given container.
 */
template <typename Container>
size_t GetContainerHashCode(const Container& container) {
  return GetContainerHashCode(std::begin(container), std::end(container));
}

}; // namespace caf

#endif
