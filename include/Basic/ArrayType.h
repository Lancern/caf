#ifndef CAF_ARRAY_TYPE_H
#define CAF_ARRAY_TYPE_H

#include "Basic/Type.h"
#include "Basic/CAFStore.h"

namespace caf {

/**
 * @brief Represents type of an array.
 *
 */
class ArrayType : public Type {
public:
  /**
   * @brief Construct a new ArrayType object.
   *
   * @param store the store holding the object.
   * @param size the number of elements in the array.
   * @param elementType the type of the elements in the array.
   * @param id the ID of this type.
   */
  explicit ArrayType(CAFStore* store, size_t size, CAFStoreRef<Type> elementType, uint64_t id)
    : Type { store, TypeKind::Array, id },
      _size(size),
      _elementType(elementType)
  { }

  /**
   * @brief Construct a new ArrayType object.
   *
   * @param store the store holding the object.
   * @param size the number of elements in the array.
   * @param id the ID of this type.
   */
  explicit ArrayType(CAFStore* store, size_t size, uint64_t id)
    : Type { store, TypeKind::Array, id },
      _size(size),
      _elementType { }
  { }

  /**
   * @brief Get the number of elements in the array.
   *
   * @return size_t the number of elements in the array.
   */
  size_t size() const { return _size; }

  /**
   * @brief Get the type of the elements in the array.
   *
   * @return CAFStoreRef<Type> the type of the elements in the array.
   */
  CAFStoreRef<Type> elementType() const { return _elementType; }

  /**
   * @brief Set the element's type.
   *
   * @param type the element's type.
   */
  void SetElementType(CAFStoreRef<Type> type) {
    _elementType = type;
  }

#ifdef CAF_LLVM
  /**
   * @brief Test whether the given object is an instance of ArrayType.
   *
   * @param object the object to be tested.
   * @return true if the object is an instance of ArrayType.
   * @return false if the object is not an instance of ArrayType.
   */
  static bool classof(const Type* object) {
    return object->kind() == TypeKind::Array;
  }
#endif

private:
  size_t _size;
  CAFStoreRef<Type> _elementType;
};

} // namespace caf

#endif
