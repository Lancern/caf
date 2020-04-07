#ifndef CAF_PROPERTY_RESOLVER_H
#define CAF_PROPERTY_RESOLVER_H

#include "Infrastructure/Optional.h"

#include <string>

namespace caf {

/**
 * @brief Abstract base class for resolving properties of objects.
 *
 * @tparam TargetTraits trait type describing target's type system.
 */
template <typename TargetTraits>
class PropertyResolver {
public:
  using ValueType = typename TargetTraits::ValueType;

  virtual ~PropertyResolver() = default;

  /**
   * @brief When overridden in derived classes, resolve the given property name on the given
   * target-specific object.
   *
   * @param value the object.
   * @param name name of the property.
   * @return Option<ValueType> the property value.
   */
  virtual Optional<ValueType> Resolve(ValueType value, const std::string& name) = 0;

protected:
  PropertyResolver() = default;
};

} // namespace caf

#endif
