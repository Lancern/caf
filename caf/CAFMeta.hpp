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
#include <stdexcept>

#include "json/json.hpp"

#ifdef CAF_LLVM
// We're in LLVM's source code tree. Use LLVM's RTTI mechanisms instead of the
// built-in ones.
#include "llvm/Support/Casting.h"
#endif


#ifndef CAF_ENABLE_LINK_SYMBOL
namespace {
#endif

namespace caf {


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
  explicit CAFStoreRef(CAFStore* store, size_t slot) noexcept
    : _store(store),
      _slot(slot)
  { }

  /**
   * @brief Apply type conversion on the template type arguments.
   * 
   * @tparam U the original type.
   * @param another CAFStoreRef that points to an instance of the original type.
   */
  template <typename U>
  CAFStoreRef(const CAFStoreRef<U>& another) noexcept
    : _store(another.store()),
      _slot(another.slot())
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
  inline T& operator*() const;

  /**
   * @brief Dereference the smart pointer and returns a raw pointer to the 
   * pointee.
   * 
   * @return T* raw pointer to the pointee.
   */
  inline T* operator->() const;

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

#ifndef CAF_LLVM
public:
  /**
   * @brief Deserialize a CAFStoreRef instance from the given JSON snippet.
   * 
   * @param context pointer to the store object owning the pointee.
   * @param json the JSON snippet.
   * @return CAFStoreRef<T> the deserialized object.
   */
  static CAFStoreRef<T> fromJson(CAFStore* context, const nlohmann::json& json) {
    auto slot = json.get<size_t>();
    return CAFStoreRef<T> { context, slot };
  }
#endif
};


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
   * @brief Construct a new FunctionSignature object.
   * 
   */
  explicit FunctionSignature() noexcept
    : _returnType { },
      _args { }
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
  inline nlohmann::json toJson() const noexcept;

private:

  CAFStoreRef<Type> _returnType;
  std::vector<CAFStoreRef<Type>> _args;

#ifndef CAF_LLVM
public:
  /**
   * @brief Deserialize a FunctionSignature instance from the given JSON 
   * snippet.
   * 
   * @param context CAFStore instance holding CAF related metadata.
   * @param json the JSON snippet to deserialize FunctionSignature object from.
   * @return FunctionSignature 
   */
  inline static FunctionSignature fromJson(
      CAFStore* context, const nlohmann::json& json) noexcept;
#endif
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
  explicit FunctionLike(FunctionSignature signature) noexcept
    : _signature(signature)
  { }

  /**
   * @brief Construct a new FunctionLike object.
   * 
   */
  explicit FunctionLike() noexcept
    : _signature { }
  { }

private:
  FunctionSignature _signature;

protected:
  /**
   * @brief Serialize the fields of the FunctionLike object into JSON
   * representation and populate them into the given JSON container.
   * 
   * @param object the object to be serialized.
   * @param json the JSON container.
   */
  static void populateJson(
      const FunctionLike& object, nlohmann::json& json) noexcept {
    json["signature"] = object._signature.toJson();
  }

#ifndef CAF_LLVM
  /**
   * @brief Deserialize fields of FunctionLike from the given JSON container
   * and populate them onto the given FunctionLike object.
   * 
   * @param context the CAFStore holding CAF related metadata.
   * @param object the object to be populated.
   * @param json the JSON container.
   */
  static void populateFromJson(
      CAFStore* context, FunctionLike& object, const nlohmann::json& json) 
      noexcept {
    object._signature = FunctionSignature::fromJson(context, json);
  }
#endif
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

  /**
   * @brief Set the ID of this object.
   * 
   * @param id the new ID of this object.
   */
  void setId(Id id) noexcept {
    _id = id;
  }

private:
  Id _id;

protected:
  /**
   * @brief Serialize the object ID of the given object and populate it into the
   * given JSON container.
   * 
   * @param object the object to serialize.
   * @param json the JSON container.
   */
  static void populateJson(
      const Identity<Concrete, Id, IdAllocator>& object, 
      nlohmann::json& json) noexcept {
    json["id"] = object._id;
  }

#ifndef CAF_LLVM
  /**
   * @brief Deserialize object ID from the given JSON snippet and populate it
   * onto the given object.
   * 
   * @param object the object to populate object ID onto.
   * @param json the JSON snippet.
   */
  static void populateFromJson(
      Identity<Concrete, Id, IdAllocator>& object, const nlohmann::json& json)
      noexcept {
    object._id = json.get<Id>();
  }
#endif
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
inline std::string toString(TypeKind typeKind) noexcept {
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

#ifndef CAF_LLVM
/**
 * @brief Parse the given string into corresponding TypeKind value.
 * 
 * @param s the string value to parse.
 * @return TypeKind the corresponding TypeKind value.
 * @throw std::invalid_argument if the given string is not a valid TypeKind
 * representation.
 */
inline TypeKind parseTypeKind(const std::string& s) {
  if (s == "Bits") {
    return TypeKind::Bits;
  } else if (s == "Pointer") {
    return TypeKind::Pointer;
  } else if (s == "Array") {
    return TypeKind::Array;
  } else if (s == "Struct") {
    return TypeKind::Struct;
  } else {
    throw std::invalid_argument { "Invalid string representation of TypeKind." };
  }
}
#endif

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
   * @brief When overridden in derived classes, convert this Type object into 
   * JSON representation.
   * 
   * @return nlohmann::json the JSON representation of this Type object.
   */
  virtual nlohmann::json toJson() const noexcept = 0;

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

#ifndef CAF_LLVM
public:
  /**
   * @brief Deserialize a polymorphic Type instance from the given JSON snippet.
   * 
   * The serialized Type instance will be added to the given CAFStore.
   * 
   * @param store CAFStore instance holding CAF related metadata.
   * @param json the JSON snippet.
   * @return CAFStoreRef<Type> pointer to the deserialized object.
   */
  inline static CAFStoreRef<Type> fromJson(
      CAFStore* context, const nlohmann::json& json) noexcept;
#endif

protected:
  /**
   * @brief Serialize the given Type object and populate its fields into the 
   * given JSON container.
   * 
   * @param object the object to be serialized.
   * @param json the JSON container.
   */
  static void populateJson(const Type& object, nlohmann::json& json) noexcept {
    Identity<Type, uint64_t>::populateJson(object, json);
    json["kind"] = toString(object._kind);
  }

#ifndef CAF_LLVM
  /**
   * @brief Deserialize the members of the given Type object from the given JSON
   * snippet and populate them onto the object.
   * 
   * @param object the object to be populated.
   * @param json the JSON snippet.
   */
  static void populateFromJson(
      Type& object, const nlohmann::json& json) noexcept {
    Identity<Type, uint64_t>::populateFromJson(object, json);
    object._kind = parseTypeKind(json["kind"].get<std::string>());
  }
#endif
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

#ifdef CAF_LLVM
public:
  /**
   * @brief Test whether the given Type object is an instance of NamedType.
   * 
   * This function is used by LLVM's RTTI implementation.
   * 
   * @param object the object to be tested.
   * @return true if the object is an instance of NamedType.
   * @return false if the object is not an instance of NamedType.
   */
  static bool classof(const Type* object) noexcept {
    return object->kind() == TypeKind::Bits ||
        object->kind() == TypeKind::Struct;
  }
#endif

protected:
  /**
   * @brief Serialize the fields of the given object into JSON representation
   * and populate them into the given JSON container.
   * 
   * @param object the object to be serialized.
   * @param json the JSON container.
   */
  static void populateJson(
      const NamedType& object, nlohmann::json& json) noexcept {
    Type::populateJson(object, json);
    json["name"] = object._name;
  }

#ifndef CAF_LLVM
  /**
   * @brief Deserialize fields of NamedType instance from the given JSON 
   * container and populate them onto the given object.
   * 
   * @param object the existing NamedType object. 
   * @param json the JSON container.
   */
  static void populateFromJson(
      NamedType& object, const nlohmann::json& json) noexcept {
    Type::populateFromJson(object, json);
    object._name = json["name"].get<std::string>();
  }
#endif
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
  nlohmann::json toJson() const noexcept override {
    auto json = nlohmann::json::object();
    populateJson(*this, json);

    return json;
  }

private:
  /**
   * @brief Construct a new BitsType object.
   * 
   * @param store the store holding this object.
   */
  explicit BitsType(CAFStore* store) noexcept
    : NamedType { store, std::string(), TypeKind::Bits },
      _size(0)
  { }

  size_t _size;

#ifdef CAF_LLVM
public:
  /**
   * @brief Test whether the given Type object is an instance of BitsType.
   * 
   * @param object the object to be tested.
   * @return true if the object is an instance of BitsType.
   * @return false if the object is not an instance of BitsType.
   */
  static bool classof(const Type* object) noexcept {
    return object->kind() == TypeKind::Bits;
  }
#endif

#ifndef CAF_LLVM
public:
  /**
   * @brief Deserialize an instance of BitsType from the given JSON container.
   * 
   * The deserialized object will be added to the given CAFStore.
   * 
   * @param context CAFStore instance holding CAF related metadata.
   * @param json the JSON container.
   * @return std::unique_ptr<BitsType> the deserialized object.
   */
  inline static CAFStoreRef<BitsType> fromJson(
      CAFStore* context, const nlohmann::json& json) noexcept;
#endif

protected:
  /**
   * @brief Serialize the given object's fields into JSON representation and
   * populate them into the given JSON container.
   * 
   * @param object the object to be serialized.
   * @param json the JSON container.
   */
  static void populateJson(
      const BitsType& object, nlohmann::json& json) noexcept {
    NamedType::populateJson(object, json);
    json["size"] = object._size;
  }

#ifndef CAF_LLVM
  /**
   * @brief Deserialize fields of BitsType from the given JSON container and 
   * populate them onto the given BitsType object.
   * 
   * @param object the object to be populated.
   * @param json the JSON container.
   */
  static void populateFromJson(
      BitsType& object, const nlohmann::json& json) noexcept {
    NamedType::populateFromJson(object, json);
    object._size = json["size"].get<size_t>();
  }
#endif
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
   * @brief Serialize this PointerType object into JSON representation.
   * 
   * @return nlohmann::json JSON representation of this PointerType object.
   */
  nlohmann::json toJson() const noexcept override {
    auto json = nlohmann::json::object();
    populateJson(*this, json);

    return json;
  }

private:
  /**
   * @brief Construct a new PointerType object.
   * 
   * @param store the CAFStore instance holding this object.
   */
  explicit PointerType(CAFStore* store) noexcept
    : Type { store, TypeKind::Pointer },
      _pointeeType { }
  { }

  CAFStoreRef<Type> _pointeeType;

#ifdef CAF_LLVM
public:
  /**
   * @brief Test whether the given object is an instance of PointerType.
   * 
   * @param object the object to be tested.
   * @return true if the given object is an instance of PointerType.
   * @return false if the given object is not an instance of PointerType.
   */
  static bool classof(const Type* object) noexcept {
    return object->kind() == TypeKind::Pointer;
  }
#endif

#ifndef CAF_LLVM
public:
  /**
   * @brief Deserialize an instance of PointerType from the given JSON 
   * container.
   * 
   * The deserialized object will be added to the given CAFStore.
   * 
   * @param context the CAFStore instance holding the deserialized object.
   * @param json the JSON container.
   * @return CAFStoreRef<PointerType> pointer to the deserialized object.
   */
  inline static CAFStoreRef<PointerType> fromJson(
      CAFStore* context, const nlohmann::json& json) noexcept;
#endif

protected:
  /**
   * @brief Serialize the fields of the given object into JSON representation
   * and populate them into the given JSON container.
   * 
   * @param object the object to be serialized.
   * @param json the JSON container.
   */
  static void populateJson(
      const PointerType& object, nlohmann::json& json) noexcept {
    Type::populateJson(object, json);
    json["pointee"] = object._pointeeType.toJson();
  }

#ifndef CAF_LLVM
  /**
   * @brief Deserialize the fields of PointerType from the given JSON container
   * and populate them onto the given object.
   * 
   * @param object the object to be populated.
   * @param json the JSON container.
   */
  static void populateFromJson(
      PointerType& object, const nlohmann::json& json) noexcept {
    Type::populateFromJson(object, json);
    object._pointeeType = CAFStoreRef<PointerType>::fromJson(
        object.store(), json);
  }
#endif
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
   * @brief Serialize this ArrayType object into JSON representation.
   * 
   * @return nlohmann::json JSON representation of this ArrayType object.
   */
  nlohmann::json toJson() const noexcept override {
    auto json = nlohmann::json::object();
    populateJson(*this, json);

    return json;
  }

private:
  /**
   * @brief Construct a new ArrayType object.
   * 
   * @param store CAFStore instance holding this object.
   */
  explicit ArrayType(CAFStore* store) noexcept
    : Type { store, TypeKind::Array },
      _size(0),
      _elementType { }
  { }

  size_t _size;
  CAFStoreRef<Type> _elementType;

#ifdef CAF_LLVM
public:
  /**
   * @brief Test whether the given object is an instance of ArrayType.
   * 
   * @param object the object to be tested.
   * @return true if the object is an instance of ArrayType.
   * @return false if the object is not an instance of ArrayType.
   */
  static bool classof(const Type* object) noexcept {
    return object->kind() == TypeKind::Array;
  }
#endif

#ifndef CAF_LLVM
public:
  /**
   * @brief Deserialize an instance of ArrayType from the given JSON container.
   * 
   * The deserialized object will be added to the given CAFStore.
   * 
   * @param context CAFStore holding the deserialized object.
   * @param json the JSON container.
   * @return CAFStoreRef<ArrayType> pointer to the deserialized object.
   */
  inline static CAFStoreRef<ArrayType> fromJson(
      CAFStore* context, const nlohmann::json& json) noexcept;
#endif

protected:
  /**
   * @brief Serialize the given object and populate the fields into the given
   * JSON container.
   * 
   * @param object the object to be serialized.
   * @param json the JSON container.
   */
  static void populateJson(
      const ArrayType& object, nlohmann::json& json) noexcept {
    Type::populateJson(object, json);
    json["size"] = object._size;
    json["elementType"] = object._elementType.toJson();
  }

#ifndef CAF_LLVM
  /**
   * @brief Deserialize fields of ArrayType from the given JSON container and
   * populate them onto the given object.
   * 
   * @param object the object to be populated.
   * @param json the JSON container.
   */
  static void populateFromJson(
      ArrayType& object, const nlohmann::json& json) noexcept {
    Type::populateFromJson(object, json);
    object._size = json["size"].get<size_t>();
    object._elementType = CAFStoreRef<Type>::fromJson(
        object.store(), json["elementType"]);
  }
#endif
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
   * @brief Serialize this StructType object into JSON representation.
   * 
   * @return nlohmann::json JSON representation of this StructType object.
   */
  nlohmann::json toJson() const noexcept override {
    auto json = nlohmann::json::object();
    populateJson(*this, json);

    return json;
  }

private:
  /**
   * @brief Construct a new StructType object.
   * 
   * @param store CAFStore holding this object.
   */
  explicit StructType(CAFStore* store) noexcept
    : NamedType { store, std::string(), TypeKind::Struct },
      _activators { },
      _fieldTypes { }
  { }

  std::vector<std::unique_ptr<Activator>> _activators;
  std::vector<CAFStoreRef<Type>> _fieldTypes;

#ifdef CAF_LLVM
public:
  /**
   * @brief Test whether the given object is an instance of StructType.
   * 
   * @param object the object to be tested.
   * @return true if the given object is an instance of StructType.
   * @return false if the given object is not an instance of StructType.
   */
  static bool classof(const Type* object) noexcept {
    return object->kind() == TypeKind::Struct;
  }
#endif

#ifndef CAF_LLVM
public:
  /**
   * @brief Deserialize an instance of StructType from the given JSON container.
   * 
   * The deserialized object will be added to the given CAFStore.
   * 
   * @param context the CAFStore holding the deserialized object.
   * @param json the JSON container.
   * @return CAFStoreRef<StructType> pointer to the deserialized object.
   */
  inline static CAFStoreRef<StructType> fromJson(
      CAFStore* context, const nlohmann::json& json) noexcept;
#endif

protected:
  /**
   * @brief Serialize the fields of the given StructType instance into JSON
   * and populate them into the given JSON container.
   * 
   * @param object the StructType object to be serialized.
   * @param json the JSON container.
   */
  inline static void populateJson(
      const StructType& object, nlohmann::json& json) noexcept;

#ifndef CAF_LLVM
  /**
   * @brief Deserialize fields of StructType from the given JSON container and
   * populate them onto the given object.
   * 
   * @param object the object to be populated.
   * @param json the JSON container.
   */
  inline static void populateFromJson(
      StructType& object, const nlohmann::json& json) noexcept;
#endif
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
inline std::string toString(ActivatorKind activatorKind) noexcept {
  switch (activatorKind) {
    case ActivatorKind::Constructor:
      return "Constructor";
    case ActivatorKind::Factory:
      return "Factory";
    default:
      return "Unknown";
  }
}

#ifndef CAF_LLVM
/**
 * @brief Parse an ActivatorKind value from its string representation.
 * 
 * @param s string representation of an ActivatorKind value.
 * @return ActivatorKind the corresponding ActivatorKind value of the given
 * string representation.
 */
inline ActivatorKind parseActivatorKind(const std::string& s) {
  if (s == "Constructor") {
    return ActivatorKind::Constructor;
  } else if (s == "Factory") {
    return ActivatorKind::Factory;
  } else {
    throw std::invalid_argument { 
        "Invalid string representation of ActivatorKind." };
  }
}
#endif

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
      FunctionSignature signature) noexcept
    : FunctionLike { signature },
      CAFStoreManaged { store },
      _kind(kind),
      _constructingType(constructingType),
      _name(std::move(name))
  { }

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
  nlohmann::json toJson() const noexcept {
    auto json = nlohmann::json::object();
    populateJson(*this, json);

    return json;
  }

private:
  /**
   * @brief Construct a new Activator object.
   * 
   * @param store CAFStore instance holding this object.
   */
  explicit Activator(CAFStore* store) noexcept
    : FunctionLike { },
      CAFStoreManaged { store },
      _kind(ActivatorKind::Constructor),
      _constructingType { },
      _name { }
  { }

  ActivatorKind _kind;
  CAFStoreRef<Type> _constructingType;
  std::string _name;

#ifndef CAF_LLVM
public:
  /**
   * @brief Deserialize an instance of Activator from the given JSON container.
   * 
   * @param context the CAFStore holding CAF related metadata.
   * @param json the JSON container.
   * @return std::unique_ptr<Activator> deserialized object.
   */
  static std::unique_ptr<Activator> fromJson(
      CAFStore* context, const nlohmann::json& json) noexcept {
    auto object = std::unique_ptr<Activator> { new Activator(context) };
    populateFromJson(*object, json);

    return object;
  }
#endif

private:
  /**
   * @brief Serialize fields of the given Activator object into JSON
   * representation and populate them into the given JSON container.
   * 
   * @param object the object to be serialized.
   * @param json the JSON container.
   */
  static void populateJson(
      const Activator& object, nlohmann::json& json) noexcept {
    Identity<Activator, uint64_t>::populateJson(object, json);
    FunctionLike::populateJson(object, json);
    json["kind"] = toString(object._kind);
    json["name"] = object._name;
    json["constructingType"] = object._constructingType.toJson();
  }

#ifndef CAF_LLVM
  /**
   * @brief Deserialize fields of an Activator instance from the given JSON
   * container and populate them onto the given object.s
   * 
   * @param object the object to be populated.
   * @param json the JSON container.
   */
  static void populateFromJson(
      Activator& object, const nlohmann::json& json) noexcept {
    Identity<Activator, uint64_t>::populateFromJson(object, json);
    FunctionLike::populateFromJson(object.store(), object, json);
    object._kind = parseActivatorKind(json["kind"].get<std::string>());
    object._name = json["name"].get<std::string>();
    object._constructingType = CAFStoreRef<Type>::fromJson(object.store(),
        json["constructingType"]);
  }
#endif
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
      FunctionSignature signature) noexcept
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
    populateJson(*this, json);

    return json;
  }

private:
  /**
   * @brief Construct a new Function object.
   * 
   * @param store the CAFStore object holding this object.
   */
  explicit Function(CAFStore* store)
    : FunctionLike { },
      CAFStoreManaged { store },
      _name { }
  { }

  std::string _name;

#ifndef CAF_LLVM
public:
  /**
   * @brief Deserialize an instance of Function from the given JSON container.
   * 
   * The deserialized object will be added to the given CAFStore.
   * 
   * @param store the store holding the deserialized object.
   * @param json the JSON container.
   * @return CAFStoreRef<Function> pointer to the deserialized object.s
   */
  static CAFStoreRef<Function> fromJson(
      CAFStore* store, const nlohmann::json& json) noexcept {
    auto object = std::unique_ptr<Function> { new Function(store) };
    populateFromJson(*object, json);

    return store->addApi(std::move(object));
  }
#endif

protected:
  /**
   * @brief Serialize the given Function object into JSON representation and
   * populate it into the given JSON container.
   * 
   * @param object the object to be serialized.
   * @param json the JSON container.
   */
  static void populateJson(
      const Function& object, nlohmann::json& json) noexcept {
    FunctionLike::populateJson(object, json);
    Identity<Function, uint64_t>::populateJson(object, json);
    json["name"] = object._name;
  }

#ifndef CAF_LLVM
  /**
   * @brief Deserialize fields of Function from the given JSON container and
   * populate them onto the given object.
   * 
   * @param object the object to be populated.
   * @param json the JSON container.
   */
  static void populateFromJson(
      Function& object, const nlohmann::json& json) noexcept {
    FunctionLike::populateFromJson(object.store(), object, json);
    Identity<Function, uint64_t>::populateFromJson(object, json);
    object._name = json["name"].get<std::string>();
  }
#endif
};


/**
 * @brief Container for CAF related metadata, a.k.a. types and API definitions.
 * 
 */
class CAFStore {
public:
  template <typename T>
  friend class CAFStoreRef;
  
  /**
   * @brief Add the given object to the store. The type of the object should
   * derive from NamedType. If the ID or name of the given type already exist
   * in the store, then the given type will not be added to the store and the
   * already existed type will be returned.
   * 
   * @tparam T the type of the object to be added.
   * @param type the object to be added.
   * @return CAFStoreRef<T> pointer to the object added, or empty if failed.
   */
  template <typename T,
            typename std::enable_if<
                std::is_base_of<NamedType, T>::value,
                int
              >::type = 0
            >
  CAFStoreRef<T> addType(std::unique_ptr<T> type) {
    auto typeId = type->id();
    if (_typeIds.find(typeId) != _typeIds.end()) {
      return CAFStoreRef<T> { this, _typeIds[typeId] };
    }

    auto typeName = type->name();
    if (_typeNames.find(typeName) != _typeNames.end()) {
      return CAFStoreRef<T> { this, _typeNames[typeName] };
    }

    auto slot = static_cast<size_t>(_types.size());
    _types.push_back(std::move(type));
    _typeIds.emplace(typeId, slot);
    _typeNames.emplace(std::move(typeName), slot);

    return CAFStoreRef<T> { this, slot };
  }

  /**
   * @brief Add the given object to the store. The type of the object should 
   * derive from Type but not derive from NamedType. If the ID of the given
   * type already exists in the store, then the given type object will not be
   * added to the store and the already existed type will be returned.
   * 
   * @tparam T the type of the object to be added.
   * @param type the object to be added.
   * @return CAFStoreRef<T> pointer to the object added, or empty if failed.
   */
  template <typename T,
            typename std::enable_if<
                std::is_base_of<Type, T>::value &&
                    !std::is_base_of<NamedType, T>::value,
                int
              >::type = 0
            >
  CAFStoreRef<T> addType(std::unique_ptr<T> type) {
    auto typeId = type->id();
    if (_typeIds.find(typeId) != _typeIds.end()) {
      return CAFStoreRef<T> { this, _typeIds[typeId] };
    }

    auto slot = static_cast<size_t>(_types.size());
    _types.push_back(std::move(type));
    _typeIds.emplace(typeId, slot);

    return CAFStoreRef<T> { this, slot };
  }

  /**
   * @brief Add the given function definition to the store. If the name or the
   * ID of the given function already exists in the store, the given function
   * will not be added to the store and the already existed function will be
   * returned.
   * 
   * @param api the function definition to be added.
   * @return CAFStoreRef<Function> pointer to the added object.
   */
  CAFStoreRef<Function> addApi(std::unique_ptr<Function> api) {
    auto apiId = api->id();
    if (_apiIds.find(apiId) != _apiIds.end()) {
      return CAFStoreRef<Function> { this, _apiIds[apiId] };
    }

    auto apiName = api->name();
    if (_apiNames.find(apiName) != _apiNames.end()) {
      return CAFStoreRef<Function> { this, _apiNames[apiName] };
    }

    auto slot = static_cast<size_t>(_apis.size());
    _apis.push_back(std::move(api));
    _apiIds.emplace(apiId, slot);
    _apiNames.emplace(std::move(apiName), slot);

    return CAFStoreRef<Function> { this, slot };
  }

  /**
   * @brief Get type definitions stored and managed in this store.
   * 
   * @return const std::vector<std::unique_ptr<Type>> list of type definitions 
   * stored and managed in this store.
   */
  const std::vector<std::unique_ptr<Type>>& types() const noexcept {
    return _types;
  }

  /**
   * @brief Get API definitions stored and managed in this store.
   * 
   * @return const std::vector<std::unique_ptr<Function>> list of API
   * definitions stored and managed in this store.
   */
  const std::vector<std::unique_ptr<Function>>& apis() const noexcept {
    return _apis;
  }

  /**
   * @brief Create a BitsType object managed by this store. If the name of the
   * BitsType object given already exist in the store, then no BitsType
   * instances will be created and the already existed BitsType instancew will
   * be returned.
   * 
   * @param name the name of the bits type.
   * @param size the size of the bits type, in bytes.
   * @return CAFStoreRef<BitsType> pointer to the created object, or empty if 
   * failed.
   */
  CAFStoreRef<BitsType> createBitsType(std::string name, size_t size) {
    auto i = _typeNames.find(name);
    if (i != _typeNames.end()) {
      return CAFStoreRef<BitsType> { this, i->second };
    }

    auto type = std::make_unique<BitsType>(this, std::move(name), size);
    return addType(std::move(type));
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
   * @brief Create a StructType object managed by this store. If the name of the
   * given struct type already exist in the store, then no StructType instances
   * will be created and the already existed instance will be returned.
   * 
   * @param name the name of the struct.
   * @return CAFStoreRef<StructType> pointer to the created object, or empty if 
   * failed.
   */
  CAFStoreRef<StructType> createStructType(std::string name) {
    auto i = _typeNames.find(name);
    if (i != _typeNames.end()) {
      return CAFStoreRef<StructType> { this, i->second };
    }

    auto type = std::make_unique<StructType>(this, std::move(name));
    return addType(std::move(type));
  }

  /**
   * @brief Create an unnamed struct type and add it to this store.
   * 
   * @return CAFStoreRef<StructType> the created unnamed struct type.
   */
  CAFStoreRef<StructType> createUnnamedStructType() {
    auto type = std::make_unique<StructType>(this, "");
    return addType(std::unique_ptr<Type> { std::move(type) });
  }

  /**
   * @brief Test whether a type with the given name exists in the store.
   * 
   * @param name the name of the type to find.
   * @return true if the type with the given name exists in the store.
   * @return false if the type with the given name does not exist in the store.
   */
  bool containsType(const std::string& name) const noexcept {
    return _typeNames.find(name) != _typeNames.end();
  }

  /**
   * @brief Get the type with the given name in the store.
   * 
   * @param name the name of the type.
   * @return CAFStoreRef<Type> pointer to the type, or empty if the name cannot
   * be found.
   */
  CAFStoreRef<Type> getType(const std::string& name) noexcept {
    auto i = _typeNames.find(name);
    if (i != _typeNames.end()) {
      return CAFStoreRef<Type> { this, i->second };
    } else {
      return CAFStoreRef<Type> { };
    }
  }

  /**
   * @brief Create a Function object representing an API in this store. If the
   * name of the API already exists in the store, then no Function object will
   * be created and the already existed Function object will be returned.
   * 
   * @param name the name of the function.
   * @param signature the signature of the function.
   * @return CAFStoreRef<Function> pointer to the created object, or empty if
   * failed.
   */
  CAFStoreRef<Function> createApi(
      std::string name, const FunctionSignature& signature) {
    auto i = _apiNames.find(name);
    if (i != _apiNames.end()) {
      return CAFStoreRef<Function> { this, i->second };
    }

    auto api = std::make_unique<Function>(this, std::move(name), signature);
    return addApi(std::move(api));
  }

  /**
   * @brief Serialize the CAFStore instance into JSON representation.
   * 
   * @return nlohmann::json JSON representation of this CAFStore instance.
   */
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
  std::unordered_map<int, size_t> _typeIds;
  std::unordered_map<std::string, size_t> _typeNames;
  std::unordered_map<int, size_t> _apiIds;
  std::unordered_map<std::string, size_t> _apiNames;

  /**
   * @brief Dereference the given pointer on this store.
   * 
   * This overload is considered as a overload resolution candidate only when 
   * the type param T is derived from Type.
   * 
   * @tparam T the type of the dereferenced object.
   * @param ref the pointer to dereference.
   * @return T* raw pointer to the dereferenced object.
   */
  template <typename T,
            typename std::enable_if<
                std::is_base_of<Type, T>::value,
                int
              >::type = 0
            >
  T* deref(const CAFStoreRef<T>& ref) {
#ifdef CAF_LLVM
    // We're in LLVM context and we're forced to use LLVM's RTTI implementation.
    return llvm::dyn_cast<T>(_types[ref.slot()].get());
#else
    // We're not in LLVM context and we're free to use C++'s built-in RTTI
    // mechanisms.
    return dynamic_cast<T *>(_types[ref.slot()].get());
#endif
  }

  /**
   * @brief Dereference the given pointer on this store.
   * 
   * @param ref the pointer to dereference.
   * @return Function* raw pointer to the dereferenced Function object.
   */
  Function* deref(const CAFStoreRef<Function>& ref) {
    return _apis[ref.slot()].get();
  }

  // TODO: Implement deserialization logic of CAFStore.
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

#ifndef CAF_LLVM
FunctionSignature FunctionSignature::fromJson(
    CAFStore* context, const nlohmann::json& json) noexcept {
  FunctionSignature signature { };
  signature._returnType = CAFStoreRef<Type>::fromJson(context, json);

  for (const auto& a : json["args"]) {
    signature._args.push_back(CAFStoreRef<Type>::fromJson(context, a));
  }

  return signature;
}
#endif

#ifndef CAF_LLVM
CAFStoreRef<Type> Type::fromJson(
    CAFStore* context, const nlohmann::json& json) noexcept {
  auto kind = parseTypeKind(json["kind"].get<std::string>());
  switch (kind) {
    case TypeKind::Bits:
      return BitsType::fromJson(context, json);
    case TypeKind::Pointer:
      return PointerType::fromJson(context, json);
    case TypeKind::Array:
      return ArrayType::fromJson(context, json);
    case TypeKind::Struct:
      return StructType::fromJson(context, json);
    default:
      return CAFStoreRef<Type> { };
  }
}
#endif

#ifndef CAF_LLVM
CAFStoreRef<BitsType> BitsType::fromJson(
    CAFStore* context, const nlohmann::json& json) noexcept {
  // We cannot use std::make_unique here because the constructor used here
  // is invisible inside std::make_unique.
  auto object = std::unique_ptr<BitsType> { new BitsType(context) };
  populateFromJson(*object, json);

  return context->addType(std::move(object));
}
#endif

#ifndef CAF_LLVM
CAFStoreRef<PointerType> PointerType::fromJson(
    CAFStore* context, const nlohmann::json& json) noexcept {
  auto object = std::unique_ptr<PointerType> { new PointerType(context) };
  populateFromJson(*object, json);

  return context->addType(std::move(object));
}
#endif

#ifndef CAF_LLVM
CAFStoreRef<ArrayType> ArrayType::fromJson(
    CAFStore* context, const nlohmann::json& json) noexcept {
  auto object = std::unique_ptr<ArrayType> { new ArrayType(context) };
  populateFromJson(*object, json);

  return context->addType(std::move(object));
}
#endif

#ifndef CAF_LLVM
CAFStoreRef<StructType> StructType::fromJson(
    CAFStore* context, const nlohmann::json& json) noexcept {
  auto object = std::unique_ptr<StructType> { new StructType(context) };
  populateFromJson(*object, json);

  return context->addType(std::move(object));
}
#endif

void StructType::populateJson(
    const StructType& object, nlohmann::json& json) noexcept {
  NamedType::populateJson(object, json);
  
  auto activators = nlohmann::json::array();
  for (const auto& act : object._activators) {
    activators.push_back(act->toJson());
  }

  auto fields = nlohmann::json::array();
  for (const auto& fie : object._fieldTypes) {
    fields.push_back(fie->toJson());
  }

  json["activators"] = std::move(activators);
  json["fields"] = std::move(fields);
}

#ifndef CAF_LLVM
void StructType::populateFromJson(
    StructType& object, const nlohmann::json& json) noexcept {
  NamedType::populateFromJson(object, json);

  for (const auto& act : json["activators"]) {
    auto activator = Activator::fromJson(object.store(), act);
    object._activators.push_back(std::move(activator));
  }

  for (const auto& fie : json["fields"]) {
    auto field = Type::fromJson(object.store(), fie);
    object._fieldTypes.push_back(std::move(field));
  }
}
#endif


} // namespace caf

#ifndef CAF_ENABLE_LINK_SYMBOL
}
#endif

#endif
