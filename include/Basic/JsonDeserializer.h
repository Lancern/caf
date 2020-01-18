#ifndef CAF_JSON_DESERIALIZER_H
#define CAF_JSON_DESERIALIZER_H

#include "Basic/CAFStore.h"

#include "json/json.hpp"

namespace caf {

class Type;
class FunctionSignature;
class Function;
class Constructor;

/**
 * @brief Provide context information during JSON deserialization.
 *
 */
struct JsonDeserializerContext {
  /**
   * @brief The @see CAFStore object being deserialized.
   *
   */
  std::unique_ptr<CAFStore> store;
}; // struct JsonDeserializerContext

/**
 * @brief Deserialize CAF metadata from their JSON representation.
 *
 */
class JsonDeserializer {
public:
  /**
   * @brief Deserialize a @see CAFStore object from the given JSON container.
   *
   * @param json the JSON container.
   * @return std::unique_ptr<CAFStore> the deserialized object.
   */
  std::unique_ptr<CAFStore> DeserializeCAFStore(const nlohmann::json& json);

private:
  JsonDeserializerContext _context;

  /**
   * @brief Deserialize a @see CAFStoreRef<T> value from the given JSON container.
   *
   * @param json the JSON container.
   * @return CAFStoreRef<T> the deserialized object.
   */
  template <typename T>
  CAFStoreRef<T> DeserializeCAFStoreRef(const nlohmann::json& json) const {
    return CAFStoreRef<T> { _context.store.get(), json.get<size_t>() };
  }

  /**
   * @brief Deserialize a @see Type value from the given JSON container.
   *
   * @param json the JSON container.
   * @return std::unique_ptr<Type> the deserialized object.
   */
  std::unique_ptr<Type> DeserializeType(const nlohmann::json& json) const;

  /**
   * @brief Deserialize a @see BitsType object from the given JSON container.
   *
   * @param json the JSON container.
   * @return std::unique_ptr<Type> the deserialized object.
   */
  std::unique_ptr<Type> DeserializeBitsType(const nlohmann::json& json) const;

  /**
   * @brief Deserialize a @see PointerType object from the given JSON container.
   *
   * @param json the JSON container.
   * @return std::unique_ptr<Type> the deserialized object.
   */
  std::unique_ptr<Type> DeserializePointerType(const nlohmann::json& json) const;

  /**
   * @brief Deserialize an @see ArrayType object from the given JSON container.
   *
   * @param json the JSON container.
   * @return std::unique_ptr<Type> the deserialized object.
   */
  std::unique_ptr<Type> DeserializeArrayType(const nlohmann::json& json) const;

  /**
   * @brief Deserialize a @see StructType object from the given JSON container.
   *
   * @param json the JSON container.
   * @return std::unique_ptr<Type> the deserialized object.
   */
  std::unique_ptr<Type> DeserializeStructType(const nlohmann::json& json) const;

  /**
   * @brief Deserialize a @see FunctionSignature object from the given JSON container.
   *
   * @param json the JSON container.
   * @return FunctionSignature the deserialized object.
   */
  FunctionSignature DeserializeFunctionSignature(const nlohmann::json& json) const;

  /**
   * @brief Deserialize a @see Function object from the given JSON container.
   *
   * @param json the JSON container.
   * @return std::unique_ptr<Function> the deserialized object.
   */
  std::unique_ptr<Function> DeserializeApiFunction(const nlohmann::json& json) const;

  /**
   * @brief Deserialize a @see Constructor object from the given JSON container.
   *
   * @param json the JSON container.
   * @return Constructor the deserialized object.
   */
  Constructor DeserializeConstructor(const nlohmann::json& json) const;
}; // class JsonDeserializer

} // namespace caf

#endif
