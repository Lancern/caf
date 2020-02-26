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
   * @param id the ID of this type.
   */
  explicit PointerType(CAFStore* store, CAFStoreRef<Type> pointeeType, uint64_t id)
    : Type { store, TypeKind::Pointer, id },
      _pointeeType(pointeeType)
  { }

  /**
   * @brief Construct a new PointerType object.
   *
   * @param store the store holding the object.
   * @param id the ID of this type.
   */
  explicit PointerType(CAFStore* store, uint64_t id)
    : Type { store, TypeKind::Pointer, id },
      _pointeeType { }
  { }

  /**
   * @brief Get the pointee's type, a.k.a. the type of the value that this
   * pointer points to.
   *
   * @return CAFStoreRef<Type> the pointee's type.
   */
  CAFStoreRef<Type> pointeeType() const { return _pointeeType; }

  /**
   * @brief Set the pointee's type.
   *
   * @param pointeeType the pointee's type.
   */
  void SetPointeeType(CAFStoreRef<Type> pointeeType) {
    _pointeeType = pointeeType;
  }

  /**
   * @brief Determine whether this pointer is a function pointer.
   *
   * @return true if this pointer is a function pointer.
   * @return false if this pointer is not a function pointer.
   */
  bool isFunctionPointer() const {
    return _pointeeType.valid() && _pointeeType->kind() == TypeKind::Function;
  }

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
  CAFStoreRef<Type> _pointeeType;
};

} // namespace caf

#endif
