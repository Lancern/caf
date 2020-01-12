#ifndef CAF_POINTER_TYPE_H
#define CAF_POINTER_TYPE_H

#include "Basic/Type.h"
#include "Basic/CAFStore.h"

namespace caf {

/**
 * @brief Represents type of a pointer.
 *
 */
class PointerType : public Type {
public:
  /**
   * @brief Construct a new PointerType object.
   *
   * @param store the store holding the object.
   * @param pointeeType the pointee's type, a.k.a. the type of the value pointed to by the pointer.
   */
  explicit PointerType(CAFStore* store, CAFStoreRef<Type> pointeeType)
    : Type { store, TypeKind::Pointer },
      _pointeeType(pointeeType)
  { }

  /**
   * @brief Get the pointee's type, a.k.a. the type of the value that this
   * pointer points to.
   *
   * @return CAFStoreRef<Type> the pointee's type.
   */
  CAFStoreRef<Type> pointeeType() const { return _pointeeType; }

#ifdef CAF_LLVM
  /**
   * @brief Test whether the given object is an instance of PointerType.
   *
   * @param object the object to be tested.
   * @return true if the given object is an instance of PointerType.
   * @return false if the given object is not an instance of PointerType.
   */
  static bool classof(const Type* object) {
    return object->kind() == TypeKind::Pointer;
  }
#endif

private:
  /**
   * @brief Construct a new PointerType object.
   *
   * @param store the CAFStore instance holding this object.
   */
  explicit PointerType(CAFStore* store)
    : Type { store, TypeKind::Pointer },
      _pointeeType { }
  { }

  CAFStoreRef<Type> _pointeeType;
};

} // namespace caf

#endif
