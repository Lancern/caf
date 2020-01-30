#include "Infrastructure/Memory.h"
#include "Basic/CAFStore.h"
#include "Basic/Type.h"
#include "Basic/NamedType.h"
#include "Basic/BitsType.h"
#include "Basic/PointerType.h"
#include "Basic/ArrayType.h"
#include "Basic/StructType.h"
#include "Basic/FunctionType.h"
#include "Basic/Function.h"

namespace caf {

CAFStore::~CAFStore() = default;

CAFStoreRef<BitsType> CAFStore::CreateBitsType(std::string name, size_t size) {
  auto i = _typeNames.find(name);
  if (i != _typeNames.end()) {
    return CAFStoreRef<BitsType> { this, i->second };
  }

  auto type = caf::make_unique<BitsType>(this, std::move(name), size);
  return AddType(std::move(type)).unchecked_dyn_cast<BitsType>();
}

CAFStoreRef<PointerType> CAFStore::CreatePointerType(CAFStoreRef<Type> pointeeType) {
  auto type = caf::make_unique<PointerType>(this, pointeeType);
  return AddType(std::move(type)).unchecked_dyn_cast<PointerType>();
}

CAFStoreRef<ArrayType> CAFStore::CreateArrayType(size_t size, CAFStoreRef<Type> elementType) {
  auto type = caf::make_unique<ArrayType>(this, size, elementType);
  return AddType(std::move(type)).unchecked_dyn_cast<ArrayType>();
}

CAFStoreRef<StructType> CAFStore::CreateStructType(std::string name) {
  auto i = _typeNames.find(name);
  if (i != _typeNames.end()) {
    return CAFStoreRef<StructType> { this, i->second };
  }

  auto type = caf::make_unique<StructType>(this, std::move(name));
  return AddType(std::move(type)).unchecked_dyn_cast<StructType>();
}

CAFStoreRef<StructType> CAFStore::CreateUnnamedStructType() {
  auto type = caf::make_unique<StructType>(this, "");
  return AddType(std::move(type)).unchecked_dyn_cast<StructType>();
}

CAFStoreRef<FunctionType> CAFStore::CreateFunctionType(
    FunctionSignature signature, uint64_t signatureId) {
  auto type = caf::make_unique<FunctionType>(this, std::move(signature), signatureId);
  return AddType(std::move(type)).unchecked_dyn_cast<FunctionType>();
}

bool CAFStore::ContainsType(const std::string& name) const {
  return _typeNames.find(name) != _typeNames.end();
}

CAFStoreRef<Type> CAFStore::GetType(const std::string& name) {
  auto i = _typeNames.find(name);
  if (i != _typeNames.end()) {
    return CAFStoreRef<Type> { this, i->second };
  } else {
    return CAFStoreRef<Type> { };
  }
}

CAFStoreRef<Function> CAFStore::CreateApi(std::string name, FunctionSignature signature) {
  auto i = _apiNames.find(name);
  if (i != _apiNames.end()) {
    return CAFStoreRef<Function> { this, i->second };
  }

  auto api = caf::make_unique<Function>(this, std::move(name), std::move(signature));
  return AddApi(std::move(api));
}

CAFStoreRef<Type> CAFStore::AddType(std::unique_ptr<Type> type) {
  auto typeId = type->id();
  if (_typeIds.find(typeId) != _typeIds.end()) {
    return CAFStoreRef<Type> { this, _typeIds[typeId] };
  }

  auto isNamedType = caf::is_a<NamedType>(type.get());
  NamedType* namedType = nullptr;
  if (isNamedType) {
    namedType = caf::dyn_cast<NamedType>(type.get());
    auto typeName = namedType->name();
    if (_typeNames.find(typeName) != _typeNames.end()) {
      return CAFStoreRef<Type> { this, _typeNames[typeName] };
    }
  }

  auto slot = static_cast<size_t>(_types.size());
  _types.push_back(std::move(type));
  _typeIds.emplace(typeId, slot);

  if (isNamedType) {
    _typeNames.emplace(namedType->name(), slot);
  }

  return CAFStoreRef<Type> { this, slot };
}

CAFStoreRef<Function> CAFStore::AddApi(std::unique_ptr<Function> api) {
  auto apiId = api->id();
  if (_apiIds.find(apiId) != _apiIds.end()) {
    return CAFStoreRef<Function> { this, _apiIds[apiId] };
  }

  auto apiName = api->name();
  if (_apiNames.find(apiName) != _apiNames.end()) {
    return CAFStoreRef<Function> { this, _apiNames[apiName] };
  }

  auto slot = static_cast<size_t>(_funcs.size());
  _funcs.push_back(std::move(api));
  _apiIds.emplace(apiId, slot);
  _apiNames.emplace(std::move(apiName), slot);

  return CAFStoreRef<Function> { this, slot };
}

} // namespace caf
