#ifndef CAF_HASH_H
#define CAF_HASH_H

#include <cstddef>
#include <functional>

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
 * @brief Get the combined hash value of all the given objects.
 *
 * @tparam T the types of the objects.
 * @tparam Last the type of the last object given in the object list.
 * @param objects the head of the object list.
 * @param last the last object.
 * @return size_t the combined hash value of all the given objects.
 */
template <typename ...T, typename Last>
size_t GetHashCode(const T& ...objects, const Last& last) {
  return CombineHash(GetHashCode(objects...), GetHashCode(last));
}

}; // namespace caf

#endif
