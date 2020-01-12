#ifndef CAF_STRUCT_TYPE_H
#define CAF_STRUCT_TYPE_H

#include "Basic/NamedType.h"
#include "Basic/Constructor.h"

namespace caf {

/**
 * @brief Represents type of a struct.
 *
 */
class StructType : public NamedType {
public:
  /**
   * @brief Construct a new StructType object.
   *
   * @param store the store holding the object.
   * @param name the name of the struct.
   */
  explicit StructType(CAFStore* store, std::string name)
    : NamedType { store, std::move(name), TypeKind::Struct },
      _ctors { }
  { }

  /**
   * @brief Construct a new StructType object.
   *
   * @param store the @see CAFStore object owning this @see StructType object.
   * @param name the name of this @see CAFStore object.
   * @param ctors constructors of this struct type.
   */
  explicit StructType(CAFStore* store, std::string name, std::vector<Constructor> ctors)
    : NamedType { store, std::move(name), TypeKind::Struct },
      _ctors(std::move(ctors))
  { }

  /**
   * @brief Get constructors of this struct.
   *
   * @return const std::vector<Constructor>& list of constructors of this struct.
   */
  const std::vector<Constructor>& ctors() const { return _ctors; }

  /**
   * @brief Add an constructor for this struct type.
   *
   * @param ctor constructor to be added.
   */
  void AddConstructor(Constructor ctor) {
    _ctors.push_back(std::move(ctor));
  }

#ifdef CAF_LLVM
  /**
   * @brief Test whether the given object is an instance of StructType.
   *
   * @param object the object to be tested.
   * @return true if the given object is an instance of StructType.
   * @return false if the given object is not an instance of StructType.
   */
  static bool classof(const Type* object) {
    return object->kind() == TypeKind::Struct;
  }
#endif

private:
  /**
   * @brief Construct a new StructType object.
   *
   * @param store CAFStore holding this object.
   */
  explicit StructType(CAFStore* store) noexcept
    : NamedType { store, std::string(), TypeKind::Struct },
      _ctors { }
  { }

  std::vector<Constructor> _ctors;
};

} // namespace caf

#endif
