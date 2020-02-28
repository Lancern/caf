#include "Infrastructure/Memory.h"
#include "Basic/CAFStore.h"
#include "Basic/Type.h"
#include "Basic/NamedType.h"
#include "Basic/BitsType.h"
#include "Basic/PointerType.h"
#include "Basic/ArrayType.h"
#include "Basic/StructType.h"
#include "Basic/FunctionType.h"
#include "Basic/AggregateType.h"
#include "Basic/Function.h"

namespace caf {

CAFStore::~CAFStore() = default;

CAFStoreRef<BitsType> CAFStore::CreateBitsType(std::string name, size_t size, uint64_t id) {
  auto type = caf::make_unique<BitsType>(this, std::move(name), size, id);
  return AddType(std::move(type)).unchecked_dyn_cast<BitsType>();
}

CAFStoreRef<PointerType> CAFStore::CreatePointerType(uint64_t id) {
  auto type = caf::make_unique<PointerType>(this, id);
  return AddType(std::move(type)).unchecked_dyn_cast<PointerType>();
}

CAFStoreRef<ArrayType> CAFStore::CreateArrayType(size_t size, uint64_t id) {
  auto type = caf::make_unique<ArrayType>(this, size, id);
  return AddType(std::move(type)).unchecked_dyn_cast<ArrayType>();
}

CAFStoreRef<StructType> CAFStore::CreateStructType(std::string name, uint64_t id) {
  auto type = caf::make_unique<StructType>(this, std::move(name), id);
  return AddType(std::move(type)).unchecked_dyn_cast<StructType>();
}

CAFStoreRef<FunctionType> CAFStore::CreateFunctionType(uint64_t signatureId, uint64_t id) {
  auto type = caf::make_unique<FunctionType>(this, signatureId, id);
  return AddType(std::move(type)).unchecked_dyn_cast<FunctionType>();
}

CAFStoreRef<AggregateType> CAFStore::CreateAggregateType(std::string name, uint64_t id) {
  auto type = caf::make_unique<AggregateType>(this, std::move(name), id);
  return AddType(std::move(type)).unchecked_dyn_cast<AggregateType>();
}

CAFStoreRef<AggregateType> CAFStore::CreateUnnamedAggregateType(uint64_t id) {
  return CreateAggregateType("", id);
}

CAFStoreRef<Function> CAFStore::CreateApi(
    std::string name, FunctionSignature signature, uint64_t id) {
  auto api = caf::make_unique<Function>(this, std::move(name), std::move(signature), id);
  return AddApi(std::move(api));
}

CAFStoreRef<Type> CAFStore::AddType(std::unique_ptr<Type> type) {
  assert(type && "type is null.");
  auto id = type->id();
  assert(_typeIds.find(id) == _typeIds.end() && "The same type ID already exists.");
  auto slot = static_cast<size_t>(_types.size());
  _types.push_back(std::move(type));
  _typeIds[id] = slot;
  return CAFStoreRef<Type> { this, slot };
}

CAFStoreRef<Function> CAFStore::AddApi(std::unique_ptr<Function> api) {
  assert(api && "api is null.");
  auto id = api->id();
  assert(_funcIds.find(id) == _funcIds.end() && "The same function ID already exists.");
  auto slot = static_cast<size_t>(_funcs.size());
  _funcs.push_back(std::move(api));
  _funcIds[id] = slot;
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

CAFStoreRef<Type> CAFStore::GetType(uint64_t id) {
  auto i = _typeIds.find(id);
  if (i == _typeIds.end()) {
    return CAFStoreRef<Type> { };
  }
  return CAFStoreRef<Type> { this, i->second };
}

CAFStoreRef<Function> CAFStore::GetApi(uint64_t id) {
  auto i = _funcIds.find(id);
  if (i == _funcIds.end()) {
    return CAFStoreRef<Function> { };
  }
  return CAFStoreRef<Function> { this, i->second };
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
