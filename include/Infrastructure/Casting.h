#ifndef CAF_CASTING_H
#define CAF_CASTING_H

#ifdef CAF_LLVM
#include "llvm/Support/Casting.h"
#endif

#include <cassert>
#include <cstddef>
#include <cstdint>
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
 * this function will trigger an assertion failure.
 */
template <typename To, typename From>
inline auto dyn_cast(From* from) -> typename propergate_cv<From, To>::Type * {
  // Check that To derives from From.
  static_assert(std::is_base_of<From, To>::value, "To does not derive from From.");

#ifdef CAF_LLVM
  return llvm::cast<To>(from);
#else
  auto ret = dynamic_cast<typename propergate_cv<From, To>::Type *>(from);
  assert(ret && "Cannot cast from From to To.");
  return ret;
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

template <size_t OutputSize, bool IsSigned>
struct MakeIntegralType { }; // struct MakeIntegralType

template <>
struct MakeIntegralType<1, false> {
  using Type = uint8_t;
}; // struct MakeIntegralType<1, false>

template <>
struct MakeIntegralType<1, true> {
  using Type = int8_t;
}; // struct MakeIntegralType<1, true>

template <>
struct MakeIntegralType<2, false> {
  using Type = uint16_t;
}; // struct MakeIntegralType<2, false>

template <>
struct MakeIntegralType<2, true> {
  using Type = int16_t;
}; // struct MakeIntegralType<2, true>

template <>
struct MakeIntegralType<4, false> {
  using Type = uint32_t;
}; // struct MakeIntegralType<4, false>

template <>
struct MakeIntegralType<4, true> {
  using Type = int32_t;
}; // struct MakeIntegralType<4, true>

template <>
struct MakeIntegralType<8, false> {
  using Type = uint64_t;
}; // struct MakeIntegralType<8, false>

template <>
struct MakeIntegralType<8, true> {
  using Type = int64_t;
}; // struct MakeIntegralType<8, true>

namespace details {

template <size_t OutputSize, typename Input>
struct IntCastHelper {
  using OutputType = typename MakeIntegralType<OutputSize, std::is_signed<Input>::value>::Type;
}; // struct IntCastHelper

} // namespace details

/**
 * @brief Cast the given integer type Input to the integer type of the given size. The sign of the
 * integer type will be kept.
 *
 * @tparam OutputSize the size of the output integer type.
 * @tparam Input type of the input integer type.
 * @param in the input integer value.
 * @return details::IntCastHelper<OutputSize, Input>::OutputType the input integer value casted to
 * the desired output integer type.
 */
template <size_t OutputSize, typename Input>
auto int_cast(Input in) -> typename details::IntCastHelper<OutputSize, Input>::OutputType {
  static_assert(std::is_integral<Input>::value, "Input is not an integral type.");
  return static_cast<typename details::IntCastHelper<OutputSize, Input>::OutputType>(in);
}

} // namespace caf

#endif
