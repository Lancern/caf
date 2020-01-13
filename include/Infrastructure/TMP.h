#ifndef CAF_TMP_H
#define CAF_TMP_H

#include <type_traits>

namespace caf {

template <typename T>
struct remove_cv_ref {
  using type = typename std::remove_reference<typename std::remove_cv<T>::type>::type;
}; // struct remove_cv_ref

} // namespace caf

#endif
