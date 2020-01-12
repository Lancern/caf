#ifndef CAF_TYPE_H
#define CAF_TYPE_H

#include <string>

#ifndef CAF_LLVM
#include <stdexcept>
#endif

#include "Basic/Identity.h"

#include "json/json.hpp"

namespace caf {

class CAFStore;

/**
 * @brief Kind of a type.
 *
 */
enum class TypeKind {
  /**
   * @brief Bits type, defined in BitsType class.
   *
   */
  Bits,

  /**
   * @brief Pointer type, defined in PointerType class.
   *
   */
  Pointer,

  /**
   * @brief Array type, defined in ArrayType class.
   *
   */
  Array,

  /**
   * @brief Struct type, defined in StructType class.
   *
   */
  Struct
};

/**
 * @brief Abstract base class of a type.
 *
 */
class Type {
public:
  /**
   * @brief Destroy this @see Type object.
   *
   */
  virtual ~Type() = default;

  /**
   * @brief Get the @see CAFStore object owning this @see Type object.
   *
   * @return CAFStore* the @see CAFStore object owning this @see Type object.
   */
  CAFStore* store() const { return _store; }

  /**
   * @brief Get the kind of the type.
   *
   * @return TypeKind kind of the typpe.
   */
  TypeKind kind() const { return _kind; }

  /**
   * @brief Get the ID of this type.
   *
   * @return uint64_t ID of this type.
   */
  uint64_t id() const { return _id.id(); }

  /**
   * @brief Set ID of this type.
   *
   * @param id ID of this type.
   */
  void SetId(uint64_t id) { _id.SetId(id); }

#ifdef CAF_LLVM
  /**
   * @brief Determine whether the given @see CAFStoreManaged object is an instance of @see Type.
   *
   * This function is used by LLVM's RTTI mechanism.
   *
   * @param entity the object to check.
   * @return true if the given object is an instance of @see Type.
   * @return false if the given object is not an instance of @see Type.
   */
  static bool classof(const CAFStoreManaged* entity) {
    return entity->entityKind() == CAFStoreEntityKind::Type;
  }
#endif

protected:
  /**
   * @brief Construct a new Type object.
   *
   * @param store the store that holds the instance.
   * @param kind the kind of the type.
   */
  explicit Type(CAFStore* store, TypeKind kind)
    : _store(store),
      _id { },
      _kind(kind)
  { }

private:
  CAFStore* _store;
  Identity<Type, uint64_t> _id;
  TypeKind _kind;
};

} // namespace caf

#endif
