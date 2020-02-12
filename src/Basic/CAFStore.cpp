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

CAFStoreRef<BitsType> CAFStore::CreateBitsType(std::string name, size_t size, uint64_t id) {
  auto i = _typeNames.find(name);
  if (i != _typeNames.end()) {
    return CAFStoreRef<BitsType> { this, i->second };
  }

  auto type = caf::make_unique<BitsType>(this, std::move(name), size, id);
  return AddType(std::move(type)).unchecked_dyn_cast<BitsType>();
}

CAFStoreRef<PointerType> CAFStore::CreatePointerType(CAFStoreRef<Type> pointeeType, uint64_t id) {
  auto type = caf::make_unique<PointerType>(this, pointeeType, id);
  return AddType(std::move(type)).unchecked_dyn_cast<PointerType>();
}

CAFStoreRef<ArrayType> CAFStore::CreateArrayType(
    size_t size, CAFStoreRef<Type> elementType, uint64_t id) {
  auto type = caf::make_unique<ArrayType>(this, size, elementType, id);
  return AddType(std::move(type)).unchecked_dyn_cast<ArrayType>();
}

CAFStoreRef<StructType> CAFStore::CreateStructType(std::string name, uint64_t id) {
  auto i = _typeNames.find(name);
  if (i != _typeNames.end()) {
    return CAFStoreRef<StructType> { this, i->second };
  }

  auto type = caf::make_unique<StructType>(this, std::move(name), id);
  return AddType(std::move(type)).unchecked_dyn_cast<StructType>();
}

CAFStoreRef<StructType> CAFStore::CreateUnnamedStructType(uint64_t id) {
  auto type = caf::make_unique<StructType>(this, "", id);
  return AddType(std::move(type)).unchecked_dyn_cast<StructType>();
}

CAFStoreRef<FunctionType> CAFStore::CreateFunctionType(uint64_t signatureId, uint64_t id) {
  auto type = caf::make_unique<FunctionType>(this, signatureId, id);
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

CAFStoreRef<FunctionType> CAFStore::GetFunctionType(uint64_t signatureId) {
  auto i = _funcTypeSignatures.find(signatureId);
  if (i == _funcTypeSignatures.end()) {
    return CAFStoreRef<FunctionType> { };
  }
  return CAFStoreRef<FunctionType> { this, i->second };
}

CAFStoreRef<Function> CAFStore::CreateApi(
    std::string name, FunctionSignature signature, uint64_t id) {
  auto i = _apiNames.find(name);
  if (i != _apiNames.end()) {
    return CAFStoreRef<Function> { this, i->second };
  }

  auto api = caf::make_unique<Function>(this, std::move(name), std::move(signature), id);
  return AddApi(std::move(api));
}

CAFStoreRef<Type> CAFStore::AddType(std::unique_ptr<Type> type) {
  auto typeId = type->id();
  if (_typeIds.find(typeId) != _typeIds.end()) {
    return CAFStoreRef<Type> { this, _typeIds[typeId] };
  }

  auto isNamedType = caf::is_a<NamedType>(type.get());
  auto isFuncType = caf::is_a<FunctionType>(type.get());
  NamedType* namedType = nullptr;
  FunctionType* funcType = nullptr;

  if (isNamedType) {
    namedType = caf::dyn_cast<NamedType>(type.get());
    auto typeName = namedType->name();
    if (_typeNames.find(typeName) != _typeNames.end()) {
      return CAFStoreRef<Type> { this, _typeNames[typeName] };
    }
  }

  if (isFuncType) {
    funcType = caf::dyn_cast<FunctionType>(type.get());
    if (_funcTypeSignatures.find(funcType->signatureId()) != _funcTypeSignatures.end()) {
      return CAFStoreRef<Type> { this, _funcTypeSignatures[funcType->signatureId()] };
    }
  }

  auto slot = static_cast<size_t>(_types.size());
  _types.push_back(std::move(type));
  _typeIds.emplace(typeId, slot);

  if (isNamedType) {
    _typeNames.emplace(namedType->name(), slot);
  }

  if (isFuncType) {
    _funcTypeSignatures.emplace(funcType->signatureId(), slot);
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

void CAFStore::AddCallbackFunction(uint64_t signatureId, size_t functionId) {
  auto i = _callbackFunctions.find(signatureId);
  if (i == _callbackFunctions.end()) {
    _callbackFunctions.emplace(signatureId, std::vector<size_t> { functionId });
    return;
  }
  i->second.push_back(functionId);
}

const std::vector<size_t>* CAFStore::GetCallbackFunctions(uint64_t signatureId) {
  auto i = _callbackFunctions.find(signatureId);
  if (i == _callbackFunctions.end()) {
    return nullptr;
  }
  return &i->second;
}

void CAFStore::SetCallbackFunctions(std::unordered_map<uint64_t, std::vector<size_t>> functions) {
  _callbackFunctions = std::move(functions);
}

} // namespace caf
