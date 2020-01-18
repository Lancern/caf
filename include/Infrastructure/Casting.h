#ifndef CAF_CASTING_H
#define CAF_CASTING_H

#ifdef CAF_LLVM
#include "llvm/Support/Casting.h"
#endif

#include <type_traits>

namespace caf {

/**
 * @brief Propergate cv qualifiers from `From` to `To`.
 *
 * @tparam From the source type whose cv qualifierss will be propergated to the target type.
 * @tparam To the target type.
 */
template <typename From, typename To>
struct propergate_cv {
  using Type = To;
};

template <typename From, typename To>
struct propergate_cv<const From, To> {
  using Type = const To;
};

template <typename From, typename To>
struct propergate_cv<volatile From, To> {
  using Type = volatile To;
};

template <typename From, typename To>
struct propergate_cv<const volatile From, To> {
  using Type = const volatile To;
};

/**
 * @brief Perform pointer-based runtime type cast downwards.
 *
 * @tparam To the target type.
 * @tparam From the source type.
 * @param from the source pointer.
 * @return propergate_cv<From, To>::Type* the output pointer. If `From` cannot be casted to `To`,
 * returns nullptr.
 */
template <typename To, typename From>
inline auto dyn_cast(From* from) -> typename propergate_cv<From, To>::Type * {
  // Check that To derives from From.
  static_assert(std::is_base_of<From, To>::value, "To does not derive from From.");

#ifdef CAF_LLVM
  return llvm::dyn_cast<To>(from);
#else
  return dynamic_cast<typename propergate_cv<From, To>::Type *>(from);
#endif
}

/**
 * @brief Perform pointer-based runtime type cast downwards.
 *
 * @tparam To the target type.
 * @tparam From the source type.
 * @param from the source pointer.
 * @return propergate_cv<From, To>::Type* the output pointer. If `From` cannot be casted to `To`,
 * the behavior is not defined.
 */
template <typename To, typename From>
inline auto dyn_cast(From& from) -> typename propergate_cv<From, To>::Type & {
  // Check that To derives from From.
  static_assert(std::is_base_of<From, To>::value, "To does not derive from From.");

#ifdef CAF_LLVM
  return llvm::cast<To>(from);
#else
  return dynamic_cast<typename propergate_cv<From, To>::Type &>(from);
#endif
}

/**
 * @brief Determine whether the type of the given object derives from the specified type.
 *
 * @tparam To the specified type to be checked against.
 * @tparam From the static type of the object pointed to by the given pointer. `From` should be a
 * base class of `To`.
 * @param from the pointer.
 * @return true if the object's type derives from type `To`.
 * @return false if the object's type does not derive from type `To`.
 */
template <typename To, typename From>
inline bool is_a(From* from) {
  // Check that To derives from From.
  static_assert(std::is_base_of<From, To>::value, "To does not derive from From.");

#ifdef CAF_LLVM
  return llvm::isa<To>(from);
#else
  return static_cast<bool>(dyn_cast<To>(from));
#endif
}

/**
 * @brief Determine whether the type of the given object derives from the specified type.
 *
 * @tparam To the specified type to be checked against.
 * @tparam From the static type of the object pointed to by the given pointer. `From` should be a
 * base class of `To`.
 * @param from the pointer.
 * @return true if the object's type derives from type `To`.
 * @return false if the object's type does not derive from type `To`.
 */
template <typename To, typename From>
inline bool is_a(From& from) {
  return is_a<To>(&from);
}

} // namespace caf

#endif
