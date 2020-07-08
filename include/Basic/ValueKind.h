#ifndef CAF_VALUE_KIND_H
#define CAF_VALUE_KIND_H

#include <cstdint>

namespace caf {

#define CAF_JS_VALUE_KIND_LIST(V) \
  V(Undefined) \
  V(Null) \
  V(Boolean) \
  V(String) \
  V(Function) \
  V(Integer) \
  V(Float) \
  V(Array)

#define CAF_VALUE_KIND_LIST(V) \
  CAF_JS_VALUE_KIND_LIST(V) \
  V(Placeholder)

/**
 * @brief Kinds of a language specific value.
 *
 */
enum class ValueKind : uint8_t {
#define DECL_ENUMERATOR(name) name,
  CAF_VALUE_KIND_LIST(DECL_ENUMERATOR)
#undef DECL_ENUMERATOR
}; // enum class ValueKind

} // namespace caf

#endif
