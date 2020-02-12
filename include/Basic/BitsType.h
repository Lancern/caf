#ifndef CAF_BITS_TYPE_H
#define CAF_BITS_TYPE_H

#include "Basic/NamedType.h"

namespace caf {

/**
 * @brief Represent types that can be constructed by directly manipulating
 * its representing raw bits. Typical C++ equivalent of BitsType are the integer
 * types.
 *
 */
class BitsType : public NamedType {
public:
  /**
   * @brief Construct a new BitsType instance.
   *
   * @param store store that holds this instance.
   * @param name the name of the type.
   * @param size the size of the type, in bytes.
   * @param id the ID of the type, in bytes.
   */
  explicit BitsType(CAFStore* store, std::string name, size_t size, uint64_t id)
    : NamedType { store, std::move(name), TypeKind::Bits, id },
      _size(size)
  { }

  /**
   * @brief Get the size of the type, in bytes.
   *
   * @return size_t size of the type, in bytes.
   */
  size_t size() const { return _size; }

#ifdef CAF_LLVM
  /**
   * @brief Test whether the given Type object is an instance of BitsType.
   *
   * This function is used by LLVM's RTTI mechanism.
   *
   * @param object the object to be tested.
   * @return true if the object is an instance of BitsType.
   * @return false if the object is not an instance of BitsType.
   */
  static bool classof(const Type* object) {
    return object->kind() == TypeKind::Bits;
  }
#endif

private:
  /**
   * @brief Construct a new BitsType object.
   *
   * @param store the store holding this object.
   * @param id the ID of this type.
   */
  explicit BitsType(CAFStore* store, uint64_t id)
    : NamedType { store, std::string(), TypeKind::Bits, id },
      _size(0)
  { }

  size_t _size;
};

} // namespace caf

#endif
