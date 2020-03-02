#ifndef CAF_AGGREGATE_TYPE_H
#define CAF_AGGREGATE_TYPE_H

#include "Basic/CAFStore.h"
#include "Basic/NamedType.h"

#include <vector>

namespace caf {

/**
 * @brief An aggregate type. Aggregate types are struct types that do not have any constructors.
 *
 */
class AggregateType : public NamedType {
public:
  /**
   * @brief Construct a new AggregateType object.
   *
   * @param store the CAFStore object holding this object.
   * @param name the name of this aggregate type.
   * @param id the ID of this type.
   */
  explicit AggregateType(CAFStore* store, std::string name, uint64_t id)
    : NamedType { store, std::move(name), TypeKind::Aggregate, id }
  { }

  /**
   * @brief Construct a new AggregateType object.
   *
   * @param store the CAFStore object holding this object.
   * @param name the name of this aggregate type.
   * @param fields the fields of this aggregate type.
   * @param id the ID of this type.
   */
  explicit AggregateType(
      CAFStore* store, std::string name, std::vector<CAFStoreRef<Type>> fields, uint64_t id)
    : NamedType { store, std::move(name), TypeKind::Aggregate, id },
      _fields(std::move(fields))
  { }

  /**
   * @brief Get the types of fields of this aggregate type.
   *
   * @return const std::vector<CAFStoreRef<Type>>& the types of fields of this aggregate type.
   */
  const std::vector<CAFStoreRef<Type>>& fields() const { return _fields; }

  /**
   * @brief Get the number of fields of this aggregate type.
   *
   * @return size_t the number of fields of this aggregate type.
   */
  size_t GetFieldsCount() const { return _fields.size(); }

  /**
   * @brief Get the type of the field at the given index.
   *
   * @param index the index of the field.
   * @return CAFStoreRef<Type> the type of the field at the given index.
   */
  CAFStoreRef<Type> GetField(size_t index) const { return _fields[index]; }

  /**
   * @brief Add the given field to this aggregate type.
   *
   * @param field the field to add.
   */
  void AddField(CAFStoreRef<Type> field) {
    _fields.push_back(field);
  }

#ifdef CAF_LLVM
  /**
   * @brief Test whether the given object is an instance of AggregateType.
   *
   * @param object the object to be tested.
   * @return true if the given object is an instance of AggregateType.
   * @return false if the given object is not an instance of AggregateType.
   */
  static bool classof(const Type* object) {
    return object->kind() == TypeKind::Aggregate;
  }
#endif

private:
  std::vector<CAFStoreRef<Type>> _fields;
};

} // namespace caf

#endif
