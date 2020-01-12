#ifndef CAF_MEMORY_H
#define CAF_MEMORY_H

#include <memory>
#include <type_traits>

namespace caf {

namespace details {

template <typename T>
struct array_traits {
  constexpr static const bool is_array = false;
  constexpr static const bool known_size = false;
};

template <typename T>
struct array_traits<T[]> {
  constexpr static const bool is_array = true;
  constexpr static const bool known_size = false;

  using element_type = T;
};

template <typename T, size_t N>
struct array_traits<T[N]> {
  constexpr static const bool is_array = true;
  constexpr static const bool known_size = true;
  constexpr static const size_t size = N;

  using element_type = T;
};

} // namespace details

template <typename T, typename ...Args,
          typename std::enable_if<
              !details::array_traits<T>::is_array,
              int
            >::type = 0
          >
std::unique_ptr<T> make_unique(Args&&... args) {
#if __cplusplus < 201402L
  // Before C++14, manually construct std::unique_ptr.
  return std::unique_ptr<T> { new T(std::forward<Args>(args)...) };
#else
  return std::make_unique<T>(std::forward<Args>(args)...);
#endif
}

template <typename T,
          typename std::enable_if<
              details::array_traits<T>::is_array && !details::array_traits<T>::known_size,
              int
            >::type = 0
          >
std::unique_ptr<T> make_unique(size_t size) {
#if __cplusplus < 201402L
  // Before C++ 14, manually construct std::unique_ptr.
  return std::unique_ptr<T> { new typename details::array_traits<T>::element_type [size] };
#else
  return std::make_unique<T>(size);
#endif
}

} // namespace caf

#endif
