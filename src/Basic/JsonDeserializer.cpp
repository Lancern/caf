#include "Infrastructure/Memory.h"
#include "Basic/Type.h"
#include "Basic/BitsType.h"
#include "Basic/PointerType.h"
#include "Basic/ArrayType.h"
#include "Basic/StructType.h"
#include "Basic/FunctionType.h"
#include "Basic/Constructor.h"
#include "Basic/JsonDeserializer.h"

namespace caf {

std::unique_ptr<CAFStore> JsonDeserializer::DeserializeCAFStore(const nlohmann::json& json) {
  _context.store = caf::make_unique<CAFStore>();

  for (const auto& typeJson : json["types"]) {
    auto type = DeserializeType(typeJson);
    _context.store->AddType(std::move(type));
  }

  for (const auto& apiJson : json["apis"]) {
    auto api = DeserializeApiFunction(apiJson);
    _context.store->AddApi(std::move(api));
  }

  _context.store->SetCallbackFunctions(
      json["callbackFuncs"].get<std::unordered_map<uint64_t, std::vector<size_t>>>());

  return std::move(_context.store);
}

std::unique_ptr<Type> JsonDeserializer::DeserializeType(const nlohmann::json& json) const {
  auto kind = static_cast<TypeKind>(json["kind"].get<int>());

  std::unique_ptr<Type> type;
  switch (kind) {
    case TypeKind::Bits:
      type = DeserializeBitsType(json);
      break;

    case TypeKind::Pointer:
      type = DeserializePointerType(json);
      break;

    case TypeKind::Array:
      type = DeserializeArrayType(json);
      break;

    case TypeKind::Struct:
      type = DeserializeStructType(json);
      break;

    case TypeKind::Function:
      type = DeserializeFunctionType(json);
      break;
  }

  if (type) {
    auto id = json["id"].get<uint64_t>();
    type->SetId(id);
  }

  return type;
}

std::unique_ptr<Type> JsonDeserializer::DeserializeBitsType(const nlohmann::json& json) const {
  auto name = json["name"].get<std::string>();
  auto size = json["size"].get<size_t>();
  return caf::make_unique<BitsType>(_context.store.get(), std::move(name), size, 0);
}

std::unique_ptr<Type> JsonDeserializer::DeserializePointerType(const nlohmann::json& json) const {
  auto pointee = DeserializeCAFStoreRef<Type>(json["pointee"]);
  return caf::make_unique<PointerType>(_context.store.get(), pointee, 0);
}

std::unique_ptr<Type> JsonDeserializer::DeserializeArrayType(const nlohmann::json& json) const {
  auto size = json["size"].get<size_t>();
  auto element = DeserializeCAFStoreRef<Type>(json["element"]);
  return caf::make_unique<ArrayType>(_context.store.get(), size, element, 0);
}

std::unique_ptr<Type> JsonDeserializer::DeserializeStructType(const nlohmann::json& json) const {
  auto name = json["name"].get<std::string>();

  std::vector<Constructor> ctors;
  for (const auto& ctorJson : json["ctors"]) {
    auto ctor = DeserializeConstructor(ctorJson);
    ctors.push_back(std::move(ctor));
  }

  return caf::make_unique<StructType>(_context.store.get(), std::move(name), std::move(ctors), 0);
}

std::unique_ptr<Type> JsonDeserializer::DeserializeFunctionType(const nlohmann::json& json) const {
  auto signature = DeserializeFunctionSignature(json["signature"]);
  auto signatureId = json["signatureId"].get<uint64_t>();
  return caf::make_unique<FunctionType>(_context.store.get(), std::move(signature), signatureId, 0);
}

FunctionSignature JsonDeserializer::DeserializeFunctionSignature(const nlohmann::json& json) const {
  auto ret = DeserializeCAFStoreRef<Type>(json["ret"]);

  std::vector<CAFStoreRef<Type>> args;
  for (const auto& argJson : json["args"]) {
    auto arg = DeserializeCAFStoreRef<Type>(argJson);
    args.push_back(arg);
  }

  return FunctionSignature { ret, std::move(args) };
}

std::unique_ptr<Function> JsonDeserializer::DeserializeApiFunction(
    const nlohmann::json& json) const {
  auto id = json["id"].get<uint64_t>();
  auto name = json["name"].get<std::string>();
  auto signature = DeserializeFunctionSignature(json["signature"]);

  return caf::make_unique<Function>(
      _context.store.get(), std::move(name), std::move(signature), id);
}

Constructor JsonDeserializer::DeserializeConstructor(const nlohmann::json& json) const {
  auto id = json["id"].get<uint64_t>();
  auto signature = DeserializeFunctionSignature(json["signature"]);

  return Constructor { std::move(signature), id };
}

} // namespace caf
