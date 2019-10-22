#ifndef CAF_META_HPP
#define CAF_META_HPP

#include <utility>
#include <cstdint>
#include <memory>
#include <iterator>
#include <string>
#include <vector>
#include <unordered_map>
#include <type_traits>

#include <json/json.hpp>


namespace caf {

#ifdef CAF_NO_EXPORTED_SYMBOL
namespace {
#endif


class FunctionSignature;
class FunctionLike;
class CAFStoreManaged;
class Type;
class NamedType;
class IntegerType;
class PointerType;
class ArrayType;
class StructType;
class Activator;
class Constructor;
class Factory;
class Function;
class CAFStore;


/**
 * @brief Represents signature of a function-like object.
 * 
 */
class FunctionSignature {
public:
  /**
   * @brief Construct a new FunctionSignature object.
   * 
   * @param returnType the return type of the function.
   * @param args the types of arguments of the function.
   */
  explicit FunctionSignature(
      CAFStoreRef<Type> returnType, std::vector<CAFStoreRef<Type>> args) 
      noexcept
    : _returnType(returnType),
      _args(std::move(args))
  { }

  /**
   * @brief Get the return type of the function.
   * 
   * @return CAFStoreRef<Type> the return type of the function.
   */
  CAFStoreRef<Type> returnType() const noexcept {
    return _returnType;
  }

  /**
   * @brief Get the types of arguments of the function.
   * 
   * @return const std::vector<CAFStoreRef<Type>> type of arguments of the function.
   */
  const std::vector<CAFStoreRef<Type>> args() const noexcept {
    return _args;
  }

  /**
   * @brief Get the JSON representation of this FunctionSignature instance.
   * 
   * @return nlohmann::json JSON representation of this object.
   */
  nlohmann::json toJson() const noexcept;

private:
  CAFStoreRef<Type> _returnType;
  std::vector<CAFStoreRef<Type>> _args;
};

/**
 * @brief Abstract base class of all function-like abstractions in CAF.
 * 
 * All function-like abstractions should have an argument list and a return
 * value.
 * 
 */
class FunctionLike {
public:
  /**
   * @brief Get the signature of the function.
   * 
   * @return const FunctionSignature& signature of the function.
   */
  const FunctionSignature& signature() const noexcept {
    return _signature;
  }

protected:
  /**
   * @brief Construct a new FunctionLike object.
   * 
   * @param signature signature of the function.
   */
  explicit FunctionLike(const FunctionSignature& signature) noexcept
    : _signature(signature)
  { }

private:
  const FunctionSignature& _signature;
};


/**
 * @brief Abstract base class for objects that are stored and managed inside
 * a CAFStore object.
 * 
 */
class CAFStoreManaged {
public:
  /**
   * @brief Get the store holding this object.
   * 
   * @return CAFStore* the store holding this object.
   */
  CAFStore* store() const noexcept {
    return _store;
  }

protected:
  /**
   * @brief Construct a new CAFStoreManaged object.
   * 
   * @param store the store holding the object.
   */
  explicit CAFStoreManaged(CAFStore* store) noexcept
    : _store(store)
  { }

private:
  CAFStore* _store;
};


/**
 * @brief Provide a smart pointer whose pointee is owned and managed by a
 * CAFStore instance.
 * 
 * @tparam T the type of the pointee.
 */
template <typename T>
class CAFStoreRef {
public:
  /**
   * @brief Construct a new CAFStoreRef object that represents an empty pointer.
   * 
   */
  explicit CAFStoreRef() noexcept
    : _store(nullptr),
      _slot(0)
  { }

  /**
   * @brief Construct a new CAFStoreRef object.
   * 
   * @param store the CAFStore object that owns the pointee.
   * @param slot the index of the slot.
   */
  explicit CAFStoreRef(CAFStore* store, size_t slot)
    : _store(store),
      _slot(slot)
  { }

  /**
   * @brief Test whether the CAFStoreRef object represents an empty reference.
   * 
   * @return true if the CAFStoreRef object represents an empty reference.
   * @return false otherwise.
   */
  bool empty() const noexcept {
    return !_store;
  }

  /**
   * @brief Get the store that owns the pointee.
   * 
   * @return CAFStore* the store that owns the pointee.
   */
  CAFStore* store() const noexcept {
    return _store;
  }

  /**
   * @brief Get the index of the slot the pointer points to.
   * 
   * @return size_t the index of the slot the pointer points to.
   */
  size_t slot() const noexcept {
    return _slot;
  }

  /**
   * @brief Dereference the pointer and returns a reference to the pointee.
   * 
   * @return T& reference to the pointee.
   */
  T& operator*() const;

  /**
   * @brief Dereference the smart pointer and returns a raw pointer to the 
   * pointee.
   * 
   * @return T* raw pointer to the pointee.
   */
  T* operator->() const;

  /**
   * @brief Get the JSON representation of this CAFStoreRef instance.
   * 
   * @tparam T the type of the pointee.
   * @return nlohmann::json JSON representation of this object.
   */
  nlohmann::json toJson() const noexcept {
    return nlohmann::json(_slot);
  }

private:
  CAFStore* _store;
  size_t _slot;
};


namespace {

/**
 * @brief Provide default logic to allocate self-increment IDs. This class is 
 * not thread-safe. Multiple instances of IdAllocator are independent, and they 
 * may produce the same IDs.
 * 
 */
template <typename T>
class DefaultIdAllocator {
  static_assert(std::is_integral<T>::value, "T should be an integral type.");

public:
  /**
   * @brief Generate next ID in ID sequence.
   * 
   * @return T the next ID.
   */
  T operator()() noexcept {
    return _id++;
  }

private:
  T _id;
}; // class IdAllocator

} // namespace <anonymous>

/**
 * @brief Abstract base class for types whose instances own unique IDs.
 * 
 * @tparam Concrete the type of the concrete types.
 * @tparam Id the type of instance IDs to be used.
 * @tparam IdAllocator the type of the ID allocator. 
 */
template <typename Concrete, 
          typename Id, 
          typename IdAllocator = DefaultIdAllocator<Id>>
class Identity {
  static IdAllocator _allocator;

public:
  /**
   * @brief Get the ID of this object.
   * 
   * @return Id ID of this object.
   */
  Id id() const noexcept {
    return _id;
  }

protected:
  /**
   * @brief Construct a new Identity object. The ID of this object will be
   * generated automatically through the specified IdAllocator.
   * 
   */
  explicit Identity() noexcept(noexcept(_allocator()))
    : _id(_allocator())
  { }

  /**
   * @brief Construct a new Identity object.
   * 
   * @param id the ID of this object.
   */
  explicit Identity(Id id) noexcept
    : _id(id)
  { }

private:
  Id _id;
};

template <typename Concrete, typename Id, typename IdAllocator>
IdAllocator Identity<Concrete, Id, IdAllocator>::_allocator { };


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
 * @brief Get the string representation of the given TypeKind value.
 * 
 * @param typeKind the TypeKind value to be serialized.
 * @return std::string the string representation of the given TypeKind value.
 */
inline std::string toString(TypeKind typeKind) {
  switch (typeKind) {
    case TypeKind::Bits:
      return "Bits";
    case TypeKind::Pointer:
      return "Pointer";
    case TypeKind::Array:
      return "Array";
    case TypeKind::Struct:
      return "Struct";
    default:
      return "Unknown";
  }
}

/**
 * @brief Abstract base class of a type.
 * 
 */
class Type : public CAFStoreManaged, public Identity<Type, uint64_t> {
public:
  /**
   * @brief Get the kind of the type.
   * 
   * @return TypeKind kind of the typpe.
   */
  TypeKind kind() const noexcept {
    return _kind;
  }

  /**
   * @brief Destroy the Type object.
   * 
   */
  virtual ~Type() = default;

  /**
   * @brief Convert the given Type object into JSON representation.
   * 
   * @param type the object to be serialized.
   * @return nlohmann::json the JSON representation of the given object.
   */
  nlohmann::json toJson() const noexcept;

protected:
  /**
   * @brief Construct a new Type object.
   * 
   * @param store the store that holds the instance.
   * @param kind the kind of the type.
   */
  explicit Type(CAFStore* store, TypeKind kind) noexcept
    : CAFStoreManaged { store },
      _kind(kind)
  { }

private:
  TypeKind _kind;
};


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
  const std::string& name() const noexcept {
    return _name;
  }

protected:
  /**
   * @brief Construct a new NamedType object.
   * 
   * @param store the store holding the object.
   * @param name the name of the type.
   * @param kind the kind of the type.
   */
  explicit NamedType(CAFStore* store, std::string name, TypeKind kind) 
      noexcept
    : Type { store, kind },
      _name(std::move(name))
  { }

private:
  std::string _name;
};


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
   */
  explicit BitsType(CAFStore* store, std::string name, size_t size) 
      noexcept
    : NamedType { store, std::move(name), TypeKind::Bits },
      _size(size)
  { }
  
  /**
   * @brief Get the size of the type, in bytes.
   * 
   * @return size_t size of the type, in bytes.
   */
  size_t size() const noexcept {
    return _size;
  }

  /**
   * @brief Populate this BitsType instance onto the given JSON container.
   * 
   * @param json the JSON container onto which this BitsType object will be
   * populated.
   */
  void populateJson(nlohmann::json& json) const noexcept {
    json["name"] = name();
    json["size"] = _size;
  }

private:
  size_t _size;
};


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
   * @param pointeeType the pointee's type, a.k.a. the type of the value pointed
   * to by the pointer.
   */
  explicit PointerType(CAFStore* store, CAFStoreRef<Type> pointeeType) 
      noexcept
    : Type { store, TypeKind::Pointer },
      _pointeeType(pointeeType)
  { }

  /**
   * @brief Get the pointee's type, a.k.a. the type of the value that this
   * pointer points to.
   * 
   * @return CAFStoreRef<Type> the pointee's type.
   */
  CAFStoreRef<Type> pointeeType() const noexcept {
    return _pointeeType;
  }

  /**
   * @brief Populate this PointerType instance onto the given JSON container.
   * 
   * @param json the JSON container onto which this PointerType object will
   * be populated.
   */
  void populateJson(nlohmann::json& json) const noexcept {
    json["pointee"] = _pointeeType.toJson();
  }

private:
  CAFStoreRef<Type> _pointeeType;
};


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
   */
  explicit ArrayType(CAFStore* store, 
      size_t size, CAFStoreRef<Type> elementType) noexcept
    : Type { store, TypeKind::Array },
      _size(size),
      _elementType(elementType)
  { }

  /**
   * @brief Get the number of elements in the array.
   * 
   * @return size_t the number of elements in the array.
   */
  size_t size() const noexcept {
    return _size;
  }

  /**
   * @brief Get the type of the elements in the array.
   * 
   * @return CAFStoreRef<Type> the type of the elements in the array.
   */
  CAFStoreRef<Type> elementType() const noexcept {
    return _elementType;
  }

  /**
   * @brief Populate this ArrayType instance onto the given JSON container.
   * 
   * @param json the JSON container onto which this ArrayType object will be
   * populated.
   */
  void populateJson(nlohmann::json& json) const noexcept {
    json["size"] = _size;
    json["element"] = _elementType.toJson();
  }

private:
  size_t _size;
  CAFStoreRef<Type> _elementType;
};


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
  explicit StructType(CAFStore* store, std::string name) noexcept
    : NamedType { store, std::move(name), TypeKind::Struct },
      _activators { },
      _fieldTypes { }
  { }
  
  /**
   * @brief Get activators of this struct.
   * 
   * @return const std::vector<std::unique_ptr<Activator>>& list of activators
   * of this struct.
   */
  const std::vector<std::unique_ptr<Activator>>& activators() const noexcept {
    return _activators;
  }

  /**
   * @brief Get types of public fields in this struct.
   * 
   * @return const std::vector<CAFStoreRef<Type>>& list of types of public fields
   * in this struct.
   */
  const std::vector<CAFStoreRef<Type>>& fieldTypes() const noexcept {
    return _fieldTypes;
  }

  /**
   * @brief Add an activator for this type.
   * 
   * @param activator activator to be added.
   */
  void addActivator(std::unique_ptr<Activator> activator) noexcept {
    _activators.push_back(std::move(activator));
  }

  /**
   * @brief Add a public field to this struct type.
   * 
   * @param field the public field to be added.
   */
  void addField(CAFStoreRef<Type> field) noexcept {
    _fieldTypes.push_back(field);
  }

  /**
   * @brief Populate this StructType instance onto the given JSON container.
   * 
   * @param json the JSON container onto which this StructType object will be
   * populated.
   */
  void populateJson(nlohmann::json& json) const noexcept {
    auto activators = nlohmann::json::array();
    for (const auto& act : _activators) {
      activators.push_back(act->toJson());
    }

    auto fields = nlohmann::json::array();
    for (const auto& fie : _fieldTypes) {
      fields.push_back(fie->toJson());
    }

    json["activators"] = std::move(activators);
    json["fields"] = std::move(fields);
  }

private:
  std::vector<std::unique_ptr<Activator>> _activators;
  std::vector<CAFStoreRef<Type>> _fieldTypes;
};


/**
 * @brief Kind of activator.
 * 
 */
enum class ActivatorKind {
  Constructor,
  Factory
}; // enum class ActivatorKind

/**
 * @brief Get the string representation of the given ActivatorKind value.
 * 
 * @param activatorKind the ActivatorKind value to be serialized.
 * @return std::string the string representation of the given ActivatorKind
 * value.
 */
inline std::string toString(ActivatorKind activatorKind) {
  switch (activatorKind) {
    case ActivatorKind::Constructor:
      return "Constructor";
    case ActivatorKind::Factory:
      return "Factory";
    default:
      return "Unknown";
  }
}

/**
 * @brief Abstract base class of activators. 
 * 
 * Activators are functions that creates or initializes instances of some type. 
 * Typical activators in CAF are constructors and factory functions, which are 
 * specialized in Constructor class and Factory class, respectively.
 * 
 */
class Activator : public FunctionLike, 
                  public CAFStoreManaged, 
                  public Identity<Activator, uint64_t> {
public:
  /**
   * @brief Destroy the Activator object.
   * 
   */
  virtual ~Activator();

  /**
   * @brief Get the ID of the activator.
   * 
   * @return id_t the ID of the activator.
   */
  id_t id() const noexcept {
    return _id;
  }

  /**
   * @brief Get the type that this activator constructs.
   * 
   * @return CAFStoreRef<Type> the type that this activator constructs.
   */
  CAFStoreRef<Type> constructingType() const noexcept {
    return _constructingType;
  }

  /**
   * @brief Get the name of the activator.
   * 
   * @return const std::string& the name of the activator.
   */
  const std::string& name() const noexcept {
    return _name;
  }

  /**
   * @brief Get the kind of the activator.
   * 
   * @return ActivatorKind the kind of the activator.
   */
  ActivatorKind kind() const noexcept {
    return _kind;
  }

  /**
   * @brief Get the JSON representation of this Activator instance.
   * 
   * @return nlohmann::json JSON representation of this Activator object.
   */
  nlohmann::json toJson() const noexcept;

protected:
  /**
   * @brief Construct a new Activator object.
   * 
   * @param store the store holding this object.
   * @param constructingType the type of the object constructed by the activator.
   * @param kind the kind of the activator.
   * @param name the name of the activator.
   * @param signature the signature of the activator.
   */
  explicit Activator(
      CAFStore* store,
      CAFStoreRef<Type> constructingType, 
      ActivatorKind kind, 
      std::string name, 
      const FunctionSignature& signature) noexcept
    : FunctionLike { signature },
      CAFStoreManaged { store },
      _constructingType(constructingType),
      _kind(kind),
      _name(std::move(name))
  { }

private:
  id_t _id;
  CAFStoreRef<Type> _constructingType;
  ActivatorKind _kind;
  std::string _name;
};


/**
 * @brief Represents a constructor.
 * 
 */
class Constructor : public Activator {
public:
  // TODO: Refactor here to ellide constructingType parameter by replacing
  // TODO: it with the first argument of signature.
  /**
   * @brief Construct a new Constructor object.
   * 
   * @param store the store holding this object.
   * @param constructingType the type that this constructor constructs.
   * @param name the name of the constructor.
   * @param signature the signature of the constructor.
   */
  explicit Constructor(
      CAFStore* store,
      CAFStoreRef<Type> constructingType,
      std::string name,
      const FunctionSignature& signature) noexcept
    : Activator { store,
                  constructingType, 
                  ActivatorKind::Constructor, 
                  std::move(name), 
                  signature }
  { }
};


/**
 * @brief Represents a factory function.
 * 
 * A factory function is a function that takes no arguments and returns an
 * instance of some type.
 * 
 */
class Factory : public Activator {
public:
  // TODO: Refactor here to ellide construtingType parameter by replacing
  // TODO: it with the return type of signature.
  /**
   * @brief Construct a new Factory object.
   * 
   * @param store the store holding this object.
   * @param constructingType the type that this factory function constructs.
   * @param name the name of the factory function.
   * @param signature the signature of the factory function.
   */
  explicit Factory(
      CAFStore* store,
      CAFStoreRef<Type> constructingType,
      std::string name,
      const FunctionSignature& signature) noexcept
    : Activator { store,
                  constructingType,
                  ActivatorKind::Factory,
                  std::move(name),
                  signature }
  { }
};


/**
 * @brief Represent an API function.
 * 
 */
class Function : public FunctionLike, 
                 public CAFStoreManaged, 
                 public Identity<Function, uint64_t> {
public:
  /**
   * @brief Construct a new Function object.
   * 
   * @param store the store holding the Function object.
   * @param name the name of the function.
   * @param signature the signature of the function.
   */
  explicit Function(CAFStore* store, std::string name, 
      const FunctionSignature& signature) noexcept
    : FunctionLike { signature },
      CAFStoreManaged { store },
      _name(std::move(name))
  { }

  /**
   * @brief Get the name of the function.
   * 
   * @return const std::string& name of the function.
   */
  const std::string& name() const noexcept {
    return _name;
  }

  /**
   * @brief Convert this Function object into JSON representation.
   * 
   * @return nlohmann::json the JSON representation of this Function object.
   */
  nlohmann::json toJson() const noexcept {
    auto json = nlohmann::json::object();
    json["id"] = id();
    json["name"] = _name;
    json["signature"] = signature().toJson();

    return json;
  }

private:
  std::string _name;
};


/**
 * @brief Container for CAF related metadata, a.k.a. types and API definitions.
 * 
 */
class CAFStore {
public:
#define ENSURE_UNIQUE_TYPE_ID(id, retValue)         \
  if (_typeIds.find(id) != _typeIds.end()) {        \
    return (retValue);                              \
  }

#define ENSURE_UNIQUE_TYPE_NAME(name, retValue)     \
  if (_typeNames.find(name) != _typeNames.end()) {  \
    return (retValue);                              \
  }

#define ENSURE_UNIQUE_FUNCTION_ID(id, retValue)     \
  if (_apiIds.find(id) != _apiIds.end()) {          \
    return (retValue);                              \
  }

#define ENSURE_UNIQUE_FUNCTION_NAME(name, retValue) \
  if (_apiNames.find(name) != _apiNames.end()) {    \
    return (retValue);                              \
  }
  
  template <typename T>
  friend class CAFStoreRef;

  /**
   * @brief Get type definitions stored and managed in this store.
   * 
   * @return const std::vector<std::unique_ptr<Type>> list of type definitions 
   * stored and managed in this store.
   */
  const std::vector<std::unique_ptr<Type>> types() const noexcept {
    return _types;
  }

  /**
   * @brief Get API definitions stored and managed in this store.
   * 
   * @return const std::vector<std::unique_ptr<Function>> list of API
   * definitions stored and managed in this store.
   */
  const std::vector<std::unique_ptr<Function>> apis() const noexcept {
    return _apis;
  }

  /**
   * @brief Create a BitsType object managed by this store.
   * 
   * @param name the name of the bits type.
   * @param size the size of the bits type, in bytes.
   * @return CAFStoreRef<BitsType> pointer to the created object, or empty if 
   * failed.
   */
  CAFStoreRef<BitsType> createBitsType(std::string name, size_t size) {
    ENSURE_UNIQUE_TYPE_NAME(name, CAFStoreRef<BitsType> { })

    auto type = std::make_unique<BitsType>(this, std::move(name), size);
    return addNamedType(std::move(type));
  }

  /**
   * @brief Create a PointerType object managed by this store.
   * 
   * @param pointeeType the type of the pointee, a.k.a. the type of the value
   * pointed to by the pointer.
   * @return CAFStoreRef<PointerType> pointer to the created object, or empty if 
   * failed.
   */
  CAFStoreRef<PointerType> createPointerType(CAFStoreRef<Type> pointeeType) {
    auto type = std::make_unique<PointerType>(this, pointeeType);
    return addType(std::move(type));
  }

  /**
   * @brief Create an ArrayType object managed by this store.
   * 
   * @param size the number of elements in the array.
   * @param elementType the type of the elements in the array.
   * @return CAFStoreRef<ArrayType> pointer to the created object, or empty if 
   * failed.
   */
  CAFStoreRef<ArrayType> createArrayType(
      size_t size, CAFStoreRef<Type> elementType) {
    auto type = std::make_unique<ArrayType>(this, size, elementType);
    return addType(std::move(type));
  }

  /**
   * @brief Create a StructType object managed by this store.
   * 
   * @param name the name of the struct.
   * @return CAFStoreRef<StructType> pointer to the created object, or empty if 
   * failed.
   */
  CAFStoreRef<StructType> createStructType(std::string name) {
    ENSURE_UNIQUE_TYPE_NAME(name, CAFStoreRef<StructType> { })

    auto type = std::make_unique<StructType>(this, std::move(name));
    return addNamedType(std::move(type));
  }

  /**
   * @brief Create a Function object representing an API in this store.
   * 
   * @param name the name of the function.
   * @param signature the signature of the function.
   * @return CAFStoreRef<Function> pointer to the created object, or empty if
   * failed.
   */
  CAFStoreRef<Function> createApi(
      std::string name, const FunctionSignature& signature) {
    ENSURE_UNIQUE_FUNCTION_NAME(name, CAFStoreRef<Function> { })

    auto api = std::make_unique<Function>(this, std::move(name), signature);
    return addApi(std::move(api));
  }

  nlohmann::json toJson() const noexcept {
    auto types = nlohmann::json::array();
    for (const auto& type : _types) {
      types.push_back(type->toJson());
    }

    auto apis = nlohmann::json::array();
    for (const auto& api : _apis) {
      apis.push_back(api->toJson());
    }

    auto ret = nlohmann::json::object();
    ret["types"] = std::move(types);
    ret["apis"] = std::move(apis);

    return ret;
  }

private:
  std::vector<std::unique_ptr<Type>> _types;
  std::vector<std::unique_ptr<Function>> _apis;
  std::unordered_map<int, int> _typeIds;
  std::unordered_map<std::string, int> _typeNames;
  std::unordered_map<int, int> _apiIds;
  std::unordered_map<std::string, int> _apiNames;

  /**
   * @brief Add the given object to the store. The type of the object should
   * derive from NamedType.
   * 
   * @tparam T the type of the object to be added.
   * @param type the object to be added.
   * @return CAFStoreRef<T> pointer to the object added, or empty if failed.
   */
  template <typename T>
  CAFStoreRef<T> addNamedType(std::unique_ptr<T> type) {
    static_assert(std::is_base_of<NamedType, T>::value,
        "T is not a derived type of NamedType.");

    auto typeId = type->id();
    ENSURE_UNIQUE_TYPE_ID(typeId, CAFStoreRef<T> { })

    auto typeName = type->name();
    ENSURE_UNIQUE_TYPE_NAME(typeName, CAFStoreRef<T> { })

    auto slot = static_cast<int>(_types.size());
    _types.push_back(std::move(type));
    _typeIds.emplace(typeId, slot);
    _typeNames.emplace(std::move(typeName), slot);

    return CAFStoreRef<T> { this, slot };
  }

  /**
   * @brief Add the given object to the store. The type of the object should 
   * derive from Type but not derive from NamedType.
   * 
   * @tparam T the type of the object to be added.
   * @param type the object to be added.
   * @return CAFStoreRef<T> pointer to the object added, or empty if failed.
   */
  template <typename T>
  CAFStoreRef<T> addType(std::unique_ptr<T> type) {
    static_assert(std::is_base_of<Type, T>::value, 
        "T is not a derived type of Type.");
    static_assert(!std::is_base_of<NamedType, T>::value,
        "T is a derived type of NamedType. Please use addNamedType instead.");

    auto typeId = type->id();
    ENSURE_UNIQUE_TYPE_ID(typeId, CAFStoreRef<T> { })

    auto slot = static_cast<int>(_types.size());
    _types.push_back(std::move(type));
    _typeIds.emplace(typeId, slot);

    return CAFStoreRef<T> { this, slot };
  }

  /**
   * @brief Add the given Type object into the store, with polymorphic dispatch.
   * 
   * @param type the type object to be added.
   * @return CAFStoreRef<Type> pointer to the added type object.
   */
  CAFStoreRef<Type> addTypePolymorphic(std::unique_ptr<Type> type) {
    // TODO: Implement CAFStore::addTypePolymorphic(std::unique_ptr<Type>).
  }

  template <typename T>
  CAFStoreRef<T> addApi(std::unique_ptr<T> api) {
    static_assert(std::is_base_of<Function, T>::value,
        "T is not a derived type of Function.");
    
    auto raw = api.get();
    
    auto apiId = api->id();
    ENSURE_UNIQUE_FUNCTION_ID(apiId, CAFStoreRef<T> { })

    auto apiName = api->name();
    ENSURE_UNIQUE_FUNCTION_NAME(apiName, CAFStoreRef<T> { })

    auto slot = static_cast<int>(_apis.size());
    _apis.push_back(std::move(api));
    _apiIds.emplace(apiId, slot);
    _apiNames.emplace(std::move(apiName), slot);

    return CAFStoreRef<T> { this, slot };
  }

  /**
   * @brief Dereference the given pointer on this store.
   * 
   * This overload is considered as a overload resolution candidate only when 
   * the type param T is derived from Type.
   * 
   * @tparam T the type of the dereferenced object.
   * @tparam SFINAE for SFINAE use.
   * @param ref the pointer to dereference.
   * @return T* raw pointer to the dereferenced object.
   */
  template <typename T,
            typename SFINAE = typename std::enable_if<
                std::is_base_of<Type, T>::value,
                void
              >::type
            >
  T* deref(const CAFStoreRef<T>& ref) {
    return dynamic_cast<T *>(_types[ref.slot()].get());
  }

  /**
   * @brief Dereference the given pointer on this store.
   * 
   * This overload is considered as a overload resolution candidate only when 
   * the type param T is derived from Function.
   * 
   * @tparam T the type of the dereferenced object.
   * @tparam SFINAE for SFINAE use.
   * @param ref the pointer to dereference.
   * @return T* raw pointer to the dereferenced object.
   */
  template <typename T,
            typename SFINAE = typename std::enable_if<
                std::is_base_of<Function, T>::value,
                void
              >::type
            >
  T* deref(const CAFStoreRef<T>& ref) {
    return dynamic_cast<T *>(_apis[ref.slot()].get());
  }

#undef ENSURE_UNIQUE_TYPE_ID
#undef ENSURE_UNIQUE_TYPE_NAME
};


template <typename T>
T& CAFStoreRef<T>::operator*() const {
  return *_store->deref(*this);
}

template <typename T>
T* CAFStoreRef<T>::operator->() const {
  return _store->deref(*this);
}


nlohmann::json FunctionSignature::toJson() const noexcept {
  auto json = nlohmann::json::object();
  json["returnType"] = returnType().toJson();
  
  auto argsJson = nlohmann::json::array();
  for (const auto& a : args()) {
    argsJson.push_back(a.toJson());
  }

  json["args"] = std::move(argsJson);
  return json;
}

nlohmann::json Type::toJson() const noexcept  {
  auto json = nlohmann::json::object();
  json["id"] = id();
  json["kind"] = toString(_kind);

  switch (_kind) {
    case TypeKind::Bits:
      dynamic_cast<const BitsType *>(this)->populateJson(json);
      break;
    case TypeKind::Pointer:
      dynamic_cast<const PointerType *>(this)->populateJson(json);
      break;
    case TypeKind::Array:
      dynamic_cast<const ArrayType *>(this)->populateJson(json);
      break;
    case TypeKind::Struct:
      dynamic_cast<const StructType *>(this)->populateJson(json);
      break;
  }

  return json;
}

nlohmann::json Activator::toJson() const noexcept {
  auto json = nlohmann::json::object();
  json["id"] = id();
  json["kind"] = toString(_kind);
  json["name"] = name();
  json["constructingType"] = _constructingType.toJson();
  json["signature"] = signature().toJson();

  return json;
}


#ifdef CAF_NO_EXPORTED_SYMBOL
} // namespace <anonymous>
#endif

} // namespace caf

#endif
