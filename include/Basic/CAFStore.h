#ifndef CAF_CAF_STORE_H
#define CAF_CAF_STORE_H

#include "Infrastructure/Casting.h"

#include <cstddef>
#include <memory>
#include <vector>
#include <unordered_map>
#include <type_traits>

namespace caf {

class Type;
class BitsType;
class PointerType;
class ArrayType;
class StructType;
class FunctionSignature;
class Function;

class CAFStore;

/**
 * @brief A smart pointer that points to an entity inside a @see CAFStore object. @see CAFStoreRef
 * does not invalidate when new entities are added into the @see CAFStore object.
 *
 * @tparam T type of the object pointed to by this @see CAFStoreRef object.
 */
template <typename T>
class CAFStoreRef {
public:
  /**
   * @brief Construct a new CAFStoreRef object that represents an empty reference.
   *
   */
  explicit CAFStoreRef()
    : _store(nullptr),
      _slot(0)
  { }

  /**
   * @brief Construct a new CAFStoreRef object.
   *
   * @param store the @see CAFStore object containing the pointed-to object.
   * @param slot the index of the slot of the pointed-to object.
   */
  explicit CAFStoreRef(CAFStore* store, size_t slot)
    : _store(store),
      _slot(slot)
  { }

  /**
   * @brief Construct a new CAFStoreRef object from a @see CAFStoreRef object that points to a type
   * derived from `T`.
   *
   * @tparam U the type of the object that the given @see CAFStoreRef object points to.
   * @param another the @see CAFStoreRef object.
   */
  template <typename U, typename std::enable_if<std::is_base_of<T, U>::value, int>::type = 0>
  CAFStoreRef(const CAFStoreRef<U>& another)
    : _store(store),
      _slot(slot)
  { }

  /**
   * @brief Get the store containing the object pointed to by this pointer.
   *
   * @return CAFStore* pointer to the store.
   */
  CAFStore* store() const { return _store; }

  /**
   * @brief Get the index of the slot containing the object pointed to by this pointer.
   *
   * @return size_t the index of the slot.
   */
  size_t slot() const { return _slot; }

  /**
   * @brief Get the pointed-to object.
   *
   * Note that the returned pointer might invalidate when new entities are added to @see CAFStore
   * object.
   *
   * @return T* pointer to the pointed-to object.
   */
  T* get() const;

  T* operator->() const { return get(); }

  T& operator*() const { return *get(); }

  /**
   * @brief Cast the pointer downwards to the specified type. This function always succeeds. The
   * behavior is undefined if the type of the object pointed to by this pointer is not U.
   *
   * @tparam U the target type.
   * @return CAFStoreRef<U> the casted pointer.
   */
  template <typename U>
  CAFStoreRef<U> unchecked_dyn_cast() const {
    static_assert(std::is_base_of<T, U>::value, "U does not derive from T.");
    return CAFStoreRef<U> { _store, _slot };
  }

  /**
   * @brief Checks whether this pointer is valid.
   *
   * @return true if this pointer is valid.
   * @return false if this pointer is not valid.
   */
  bool valid() const { return static_cast<bool>(_store); }

  explicit operator bool() const noexcept { return valid(); }

private:
  CAFStore* _store;
  size_t _slot;
}; // class CAFStoreRef

/**
 * @brief Container for CAF related metadata, a.k.a. types and API definitions.
 *
 */
class CAFStore {
public:
  template <typename T>
  friend class CAFStoreRef;

  /**
   * @brief Get all types owned by this @see CAFStore object.
   *
   * @return const std::vector<std::unique_ptr<Type>> & a list of all types owned by this
   * @see CAFStore object.
   */
  const std::vector<std::unique_ptr<Type>>& types() const {
    return _types;
  }

  /**
   * @brief Get all API functions owned by this @see CAFStore object.
   *
   * @return const std::vector<std::unique_ptr<Function>> & a list of all API functions owned by
   * this @see CAFStore object.
   */
  const std::vector<std::unique_ptr<Function>>& funcs() const {
    return _funcs;
  }

  /**
   * @brief Create a BitsType object managed by this store. If the name of the BitsType object given
   * already exist in the store, then no BitsType instances will be created and the already existed
   * BitsType instancew will be returned.
   *
   * @param name the name of the bits type.
   * @param size the size of the bits type, in bytes.
   * @return CAFStoreRef<BitsType> pointer to the created object, or empty if failed.
   */
  CAFStoreRef<BitsType> CreateBitsType(std::string name, size_t size);

  /**
   * @brief Create a PointerType object managed by this store.
   *
   * @param pointeeType the type of the pointee, a.k.a. the type of the value pointed to by the
   * pointer.
   * @return CAFStoreRef<PointerType> pointer to the created object, or empty if failed.
   */
  CAFStoreRef<PointerType> CreatePointerType(CAFStoreRef<Type> pointeeType);

  /**
   * @brief Create an ArrayType object managed by this store.
   *
   * @param size the number of elements in the array.
   * @param elementType the type of the elements in the array.
   * @return CAFStoreRef<ArrayType> pointer to the created object, or empty if failed.
   */
  CAFStoreRef<ArrayType> CreateArrayType(size_t size, CAFStoreRef<Type> elementType);

  /**
   * @brief Create a StructType object managed by this store. If the name of the given struct type
   * already exist in the store, then no StructType instances will be created and the already
   * existed instance will be returned.
   *
   * @param name the name of the struct.
   * @return CAFStoreRef<StructType> pointer to the created object, or empty if failed.
   */
  CAFStoreRef<StructType> CreateStructType(std::string name);

  /**
   * @brief Create an unnamed struct type and add it to this store.
   *
   * @return CAFStoreRef<StructType> the created unnamed struct type.
   */
  CAFStoreRef<StructType> CreateUnnamedStructType();

  /**
   * @brief Test whether a type with the given name exists in the store.
   *
   * @param name the name of the type to find.
   * @return true if the type with the given name exists in the store.
   * @return false if the type with the given name does not exist in the store.
   */
  bool ContainsType(const std::string& name) const;

  /**
   * @brief Get the type with the given name in the store.
   *
   * @param name the name of the type.
   * @return CAFStoreRef<Type> pointer to the type, or empty if the name cannot be found.
   */
  CAFStoreRef<Type> GetType(const std::string& name);

  /**
   * @brief Create a Function object representing an API in this store. If the name of the API
   * already exists in the store, then no Function object will be created and the already existed
   * Function object will be returned.
   *
   * @param name the name of the function.
   * @param signature the signature of the function.
   * @return CAFStoreRef<Function> pointer to the created object, or empty if failed.
   */
  CAFStoreRef<Function> CreateApi(std::string name, const FunctionSignature& signature);

  /**
   * @brief Add a new type to this @see CAFStore object.
   *
   * If the name of the type already exists in this @see CAFStore object, the given type will not be
   * added to the store and the already-exist @see Type object will be returned.
   *
   * @param type the type to add.
   * @return CAFStoreRef<Type> a @see CAFStoreRef pointer to the added type.
   */
  CAFStoreRef<Type> AddType(std::unique_ptr<Type> type);

  /**
   * @brief Add a new API function to the @see CAFStore object.
   *
   * @param api the API function to add.
   * @return CAFStoreRef<Function> a @see CAFStoreRef pointer to the added API function.
   */
  CAFStoreRef<Function> AddApi(std::unique_ptr<Function> api);

private:
  std::vector<std::unique_ptr<Type>> _types;
  std::vector<std::unique_ptr<Function>> _funcs;
  std::unordered_map<int, size_t> _typeIds;
  std::unordered_map<std::string, size_t> _typeNames;
  std::unordered_map<int, size_t> _apiIds;
  std::unordered_map<std::string, size_t> _apiNames;

  /**
   * @brief Get the object at the specified slot. The type of the object should derive from
   * @see Type.
   *
   * @tparam T the type of the object.
   * @param slot the index of the slot containing the object.
   * @return T* pointer to the object.
   */
  template <typename T,
            typename std::enable_if<std::is_base_of<Type, T>::value, int>::type = 0>
  T* get(size_t slot) const {
    return _types[slot].get();
  }

  /**
   * @brief Get the object at the specified slot. The type of the object should derive from
   * @see Function.
   *
   * @tparam T the type of the object.
   * @param slot the index of the slot containing the object.
   * @return T* pointer to the object.
   */
  template <typename T,
            typename std::enable_if<std::is_same<Function, T>::value, int>::type = 0>
  T* get(size_t slot) const {
    return _funcs[slot].get();
  }
}; // class CAFStore

template <typename T>
T* CAFStoreRef<T>::get() const {
  return caf::dyn_cast<T>(_store->get<T>(_slot));
}

} // namespace caf

#endif