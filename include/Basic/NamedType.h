#ifndef CAF_NAMED_TYPE_H
#define CAF_NAMED_TYPE_H

#include "Basic/Type.h"

namespace caf {

/**
 * @brief Abstract base class of those types that have names.
 *
 */
class NamedType : public Type {
public:
  /**
   * @brief Get the name of the type.
   *
   * @return const std::string& name of the type.
   */
  const std::string& name() const { return _name; }

#ifdef CAF_LLVM
  /**
   * @brief Test whether the given Type object is an instance of NamedType.
   *
   * This function is used by LLVM's RTTI implementation.
   *
   * @param object the object to be tested.
   * @return true if the object is an instance of NamedType.
   * @return false if the object is not an instance of NamedType.
   */
  static bool classof(const Type* object) {
    return object->kind() == TypeKind::Bits ||
           object->kind() == TypeKind::Struct;
  }
#endif

protected:
  /**
   * @brief Construct a new NamedType object.
   *
   * @param store the store holding the object.
   * @param name the name of the type.
   * @param kind the kind of the type.
   */
  explicit NamedType(CAFStore* store, std::string name, TypeKind kind)
    : Type { store, kind },
      _name(std::move(name))
  { }

private:
  std::string _name;
};

} // namespace caf

#endif
