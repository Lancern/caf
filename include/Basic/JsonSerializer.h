#ifndef CAF_JSON_SERIALIZER_H
#define CAF_JSON_SERIALIZER_H

#include "json/json.hpp"

namespace caf {

class CAFStore;
template <typename T> class CAFStoreRef;
class Type;
class NamedType;
class BitsType;
class PointerType;
class ArrayType;
class StructType;
class FunctionType;
class FunctionSignature;
class Function;
class Constructor;

/**
 * @brief Serialize CAF metadata into JSON representation.
 *
 */
class JsonSerializer {
public:
  /**
   * @brief Serialize the given @see CAFStore object into JSON representation.
   *
   * @param object the object to serialize.
   * @return nlohmann::json the JSON representation of the given @see CAFStore object.
   */
  nlohmann::json Serialize(const CAFStore& object) const {
    auto jsonObject = nlohmann::json::object();
    Serialize(object, jsonObject);
    return jsonObject;
  }

private:
  /**
   * @brief Serialize the given @see CAFStore object into the given JSON container.
   *
   * @param object the @see CAFStore object to serialize.
   * @param json the JSON container.
   */
  void Serialize(const CAFStore& object, nlohmann::json& json) const;

  /**
   * @brief Serialize the given @see CAFStoreRef<T> object into the given JSON container.
   *
   * @tparam T the type of the object that is pointed to by the given @see CAFStoreRef<T> object.
   * @param object the @see CAFStoreRef<T> object to serialize.
   * @param json the JSON container.
   */
  template <typename T>
  void Serialize(const CAFStoreRef<T>& object, nlohmann::json& json) const {
    json = object.slot();
  }

  /**
   * @brief Serialize the given @see Type object into the given JSON container.
   *
   * @param object the @see Type object to serialize.
   * @param json the JSON container.
   */
  void Serialize(const Type& object, nlohmann::json& json) const;

  /**
   * @brief Serialize the given @see NamedType object into the given JSON container.
   *
   * @param object the @see NamedType object to serialize.
   * @param json the JSON container.
   */
  void Serialize(const NamedType& object, nlohmann::json& json) const;

  /**
   * @brief Serialize the given @see BitsType object into the given JSON container.
   *
   * @param object the @see BitsType object to serialize.
   * @param json the JSON container.
   */
  void Serialize(const BitsType& object, nlohmann::json& json) const;

  /**
   * @brief Serialize the given @see PointerType object into the given JSON container.
   *
   * @param object the @see PointerType object to serialize.
   * @param json the JSON container.
   */
  void Serialize(const PointerType& object, nlohmann::json& json) const;

  /**
   * @brief Serialize the given @see ArrayType object into the given JSON container.
   *
   * @param object the @see ArrayType object to serialize.
   * @param json the JSON container.
   */
  void Serialize(const ArrayType& object, nlohmann::json& json) const;

  /**
   * @brief Serialize the given @see StructType object into the given JSON container.
   *
   * @param object the @see StructType object to serialize.
   * @param json the JSON container.
   */
  void Serialize(const StructType& object, nlohmann::json& json) const;

  /**
   * @brief Serialize the given @see FunctionType object into the given JSON container.
   *
   * @param object the @see FunctionType object to serialize.
   * @param json the JSON container.
   */
  void Serialize(const FunctionType& object, nlohmann::json& json) const;

  /**
   * @brief Serialize the given @see FunctionSignature object into the given JSON container.
   *
   * @param object the @see FunctionSignature object to serialize.
   * @param json the JSON container.
   */
  void Serialize(const FunctionSignature& object, nlohmann::json& json) const;

  /**
   * @brief Serialize the given @see Function object into the given JSON container.
   *
   * @param object the @see Function object to serialize.
   * @param json the JSON container.
   */
  void Serialize(const Function& object, nlohmann::json& json) const;

  /**
   * @brief Serialize the given @see Constructor object into the given JSON container.
   *
   * @param object the @see Constructor object to serialize.
   * @param json the JSON container.
   */
  void Serialize(const Constructor& object, nlohmann::json& json) const;
};

} // namespace caf

#endif
