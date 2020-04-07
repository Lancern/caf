#ifndef CAF_V8_PROPERTY_RESOLVER_H
#define CAF_V8_PROPERTY_RESOLVER_H

#include "Targets/Common/PropertyResolver.h"
#include "Targets/V8/V8Traits.h"

namespace caf {

/**
 * @brief Implement PropertyResolver for V8 target.
 *
 */
class V8PropertyResolver : public PropertyResolver<V8Traits> {
public:
  /**
   * @brief Construct a new V8PropertyResolver object.
   *
   * @param context the V8 context.
   */
  explicit V8PropertyResolver(v8::Local<v8::Context> context)
    : _context(context)
  { }

  Optional<ValueType> Resolve(ValueType value, const std::string &name) override;

private:
  v8::Local<v8::Context> _context;
}; // class V8PropertyResolver

} // namespace caf

#endif
