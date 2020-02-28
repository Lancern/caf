#include "Infrastructure/Casting.h"
#include "Infrastructure/Memory.h"
#include "Infrastructure/Identity.h"
#include "Basic/CAFStore.h"
#include "Basic/Type.h"
#include "Basic/BitsType.h"
#include "Basic/PointerType.h"
#include "Basic/ArrayType.h"
#include "Basic/StructType.h"
#include "Basic/FunctionType.h"
#include "Basic/AggregateType.h"
#include "Basic/Function.h"
#include "Basic/Constructor.h"
#include "Extractor/ExtractorContext.h"
#include "ExtractorUtils.h"

#include "llvm/IR/Module.h"
#include "llvm/Support/FormatVariadic.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <utility>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace caf {

namespace {

template <typename AssociativeContainer>
auto find(const AssociativeContainer& container,
          const typename AssociativeContainer::key_type& key)
  -> Optional<typename AssociativeContainer::mapped_type> {
  auto i = container.find(key);
  if (i == container.end()) {
    return Optional<typename AssociativeContainer::mapped_type> { };
  } else {
    return Optional<typename AssociativeContainer::mapped_type> { i->second };
  }
}

template <typename Container>
bool exist(const Container& container, const typename Container::key_type& key) {
  return container.find(key) != container.end();
}

bool IsValidType(const llvm::Type* type) {
  return type->isVoidTy() ||
         type->isIntegerTy() ||
         type->isFloatingPointTy() ||
         type->isPointerTy() ||
         type->isFunctionTy() ||
         type->isArrayTy() ||
         type->isStructTy();
}

size_t GetTypeSize(const llvm::Type* type) {
  assert((type->isIntegerTy() || type->isFloatingPointTy()) &&
      "type is not an integer type nor a floating point type.");

  if (type->isIntegerTy()) {
    return type->getIntegerBitWidth() / 8;
  } else if (type->isFloatingPointTy()) {
    if (type->isHalfTy()) {
      return 2;
    } else if (type->isFloatTy()) {
      return 4;
    } else if (type->isDoubleTy()) {
      return 8;
    } else if (type->isX86_FP80Ty()) {
      return 10;
    } else if (type->isFP128Ty() || type->isPPC_FP128Ty()) {
      return 16;
    }
  }

  assert(false && "Unreachable code.");
  return 0;
}

std::string GetTypeName(const llvm::Type* type) {
  if (type->isIntegerTy()) {
    return llvm::formatv("i{0}", type->getIntegerBitWidth());
  } else if (type->isFloatingPointTy()) {
    if (type->isHalfTy()) {
      return "f16";
    } else if (type->isFloatTy()) {
      return "f32";
    } else if (type->isDoubleTy()) {
      return "f64";
    } else if (type->isX86_FP80Ty()) {
      return "f80";
    } else if (type->isFP128Ty()) {
      return "f128";
    } else if (type->isPPC_FP128Ty()) {
      return "f128ppc";
    } else {
      // This branch should be unreachable.
      assert(false && "Unreachable code.");
      return "";  // Make the compiler happy.
    }
  } else if (type->isStructTy()) {
    return type->getStructName();
  } else {
    return "";
  }
}

} // namespace <anonymous>

class ExtractorContext::FrozenContext {
public:
  explicit FrozenContext() = default;

  FrozenContext(const FrozenContext &) = delete;
  FrozenContext(FrozenContext &&) noexcept = default;

  FrozenContext& operator=(const FrozenContext &) = delete;
  FrozenContext& operator=(FrozenContext &&) = default;

  bool HasApiFunction(const llvm::Function* func) const {
    return exist(_apiToId, func);
  }

  void AddApiFunction(const llvm::Function* func) {
    if (HasApiFunction(func)) {
      return;
    }

    auto id = _apiIdAlloc.next();
    _apis.emplace_back(func, id);
    _apiToId.emplace(func, id);
    _idToApi.emplace(id, func);
  }

  bool HasConstructor(const llvm::Function* ctor) const {
    return exist(_ctorToId, ctor);
  }

  void AddConstructor(const llvm::Type* type, const llvm::Function* ctor) {
    if (HasConstructor(ctor)) {
      return;
    }

    auto id = _ctorIdAlloc.next();
    _ctors.emplace_back(ctor, id);
    _ctorToId.emplace(ctor, id);
    _idToCtor.emplace(id, ctor);
    _typeToCtors.emplace(type, ctor);
    _ctorToType.emplace(ctor, type);
  }

  bool HasType(const llvm::Type* type) const {
    return exist(_typeToId, type);
  }

  void AddType(const llvm::Type* type) {
    if (HasType(type)) {
      return;
    }

    auto id = _typeIdAlloc.next();
    _types.emplace_back(type, id);
    _typeToId.emplace(type, id);
    _idToType.emplace(id, type);

    if (type->isFunctionTy()) {
      auto signature = LLVMFunctionSignature::FromType(type);
      _callbackSignatures.insert(signature);
    }
  }

  bool AddCallbackFunctionCandidate(const llvm::Function* func) {
    auto signature = LLVMFunctionSignature::FromFunction(func);
    if (!exist(_callbackSignatures, signature)) {
      return false;
    }

    auto id = static_cast<size_t>(_callbackFuncs.size());
    _callbackFuncs.push_back(
        Either<const llvm::Function *, LLVMFunctionSignature>::CreateLhs(func));
    _callbackSignaturesToFuncId.emplace(signature, id);
    return true;
  }

  void PopulateLackingCallbackFunction() {
    for (const auto& signature : _callbackSignatures) {
      if (exist(_callbackSignaturesToFuncId, signature)) {
        continue;
      }

      auto id = static_cast<size_t>(_callbackFuncs.size());
      _callbackFuncs.push_back(
          Either<const llvm::Function *, LLVMFunctionSignature>::CreateRhs(signature));
      _callbackSignaturesToFuncId.emplace(signature, id);
    }
  }

  Optional<uint64_t> GetApiFunctionId(const llvm::Function* func) const {
    return find(_apiToId, func);
  }

  Optional<uint64_t> GetConstructorId(const llvm::Function* ctor) const {
    return find(_ctorToId, ctor);
  }

  Optional<uint64_t> GetTypeId(const llvm::Type* type) const {
    return find(_typeToId, type);
  }

  std::vector<const llvm::Function *> GetApiFunctions() const {
    std::vector<const llvm::Function *> funcs;
    funcs.reserve(_apis.size());
    for (const auto& i : _apis) {
      funcs.push_back(i.first);
    }
    return funcs;
  }

  Optional<const llvm::Function *> GetApiFunctionById(uint64_t id) const {
    return find(_idToApi, id);
  }

  size_t GetApiFunctionsCount() const {
    return _apis.size();
  }

  Optional<const llvm::Function *> GetConstructorById(uint64_t id) const {
    return find(_idToCtor, id);
  }

  Optional<const llvm::Type *> GetTypeById(uint64_t id) const {
    return find(_idToType, id);
  }

  size_t GetTypesCount() const {
    return _types.size();
  }

  std::vector<const llvm::Function *> GetConstructorsOfType(
      const llvm::Type* type) const {
    if (!exist(_typeToCtors, type)) {
      return std::vector<const llvm::Function *> { };
    } else {
      std::vector<const llvm::Function *> ret;
      auto r = _typeToCtors.equal_range(type);
      for (auto i = r.first; i != r.second; ++i) {
        ret.push_back(i->second);
      }
      return ret;
    }
  }

  Optional<const llvm::Type *> GetConstructingType(const llvm::Function* ctor) const {
    auto i = _ctorToType.find(ctor);
    if (i == _ctorToType.end()) {
      return Optional<const llvm::Type *> { };
    } else {
      return Optional<const llvm::Type *> { i->second };
    }
  }

  std::vector<const llvm::Function *> GetConstructors() const {
    std::vector<const llvm::Function *> ctors;
    ctors.reserve(_ctors.size());
    for (const auto& c : _ctors) {
      ctors.push_back(c.first);
    }
    return ctors;
  }

  size_t GetConstructorsCount() const {
    return _ctors.size();
  }

  auto GetCallbackFunctions() const ->
      const std::vector<Either<const llvm::Function *, LLVMFunctionSignature>> & {
    return _callbackFuncs;
  }

  std::vector<size_t> GetCallbackFunctions(LLVMFunctionSignature signature) const {
    std::vector<size_t> ret;
    auto r = _callbackSignaturesToFuncId.equal_range(signature);
    for (auto i = r.first; i != r.second; ++i) {
      ret.push_back(i->second);
    }
    return ret;
  }

  size_t GetCallbackFunctionsCount() const {
    return _callbackFuncs.size();
  }

  std::unique_ptr<CAFStore> CreateCAFStore() const {
    auto store = caf::make_unique<CAFStore>();
    CreateCAFStoreContext context { this, store.get() };
    for (auto func : _apis) {
      context.AddLLVMFunction(func.first, func.second);
    }

    return store;
  }

private:
  class CreateCAFStoreContext {
  public:
    explicit CreateCAFStoreContext(const FrozenContext* frozenContext, CAFStore* store)
      : _frozenContext(frozenContext),
        _store(store)
    { }

    CAFStoreRef<Function> AddLLVMFunction(const llvm::Function* func, uint64_t id) {
      auto funcType = func->getFunctionType();
      std::vector<CAFStoreRef<Type>> argTypes { };
      for (auto paramType : funcType->params()) {
        argTypes.push_back(AddLLVMType(paramType));
      }
      auto returnType = AddLLVMType(funcType->getReturnType());

      auto funcName = func->hasName()
          ? func->getName().str()
          : std::string("");
      FunctionSignature signature { returnType, std::move(argTypes) };

      return _store->CreateApi(std::move(funcName), std::move(signature), id);
    }

  private:
    const FrozenContext* _frozenContext;
    CAFStore* _store;

    std::unordered_map<const llvm::Type *, CAFStoreRef<Type>> _types;
    std::unordered_map<LLVMFunctionSignature, uint64_t, Hasher<LLVMFunctionSignature>> _funcTypeIds;
    IncrementIdAllocator<uint64_t> _funcSignatureIdAlloc;

    CAFStoreRef<Type> AddLLVMType(const llvm::Type* type) {
      if (type->isVoidTy()) {
        return CAFStoreRef<Type> { };
      }

      auto i = _types.find(type);
      if (i != _types.end()) {
        return i->second;
      }

      auto typeId = _frozenContext->GetTypeId(type).take();

      CAFStoreRef<Type> cafType;
      if (type->isIntegerTy() || type->isFloatingPointTy()) {
        cafType = _store->CreateBitsType(GetTypeName(type), GetTypeSize(type), typeId);
        _types.emplace(type, cafType);
      } else if (type->isPointerTy()) {
        auto pointerType = _store->CreatePointerType(typeId);
        _types.emplace(type, pointerType);
        pointerType->SetPointeeType(AddLLVMType(type->getPointerElementType()));
        cafType = pointerType;
      } else if (type->isArrayTy()) {
        auto arrayType = _store->CreateArrayType(type->getArrayNumElements(), typeId);
        _types.emplace(type, arrayType);
        arrayType->SetElementType(AddLLVMType(type->getArrayElementType()));
        cafType = arrayType;
      } else if (type->isStructTy()) {
        cafType = AddLLVMStructType(caf::dyn_cast<llvm::StructType>(type), typeId);
      } else if (type->isFunctionTy()) {
        cafType = AddLLVMFunctionType(caf::dyn_cast<llvm::FunctionType>(type), typeId);
      } else {
        // This branch should be unreachable.
        assert(false && "Unreachable branch.");
        abort();
      }

      return cafType;
    }

    CAFStoreRef<Type> AddLLVMStructType(const llvm::StructType* type, uint64_t typeId) {
      auto structType = caf::dyn_cast<llvm::StructType>(type);
      if (structType->isLiteral() || !structType->hasName()) {
        return AddLLVMStructTypeAsAggregateType(type, typeId);
      } else {
        auto ctorsLLVM = _frozenContext->GetConstructorsOfType(type);
        if (ctorsLLVM.empty()) {
          return AddLLVMStructTypeAsAggregateType(type, typeId);
        }

        auto structType = _store->CreateStructType(type->getStructName(), typeId);
        _types.emplace(type, structType);

        for (auto c : ctorsLLVM) {
          auto funcType = c->getFunctionType();
          auto ctorId = _frozenContext->GetConstructorId(c);

          std::vector<CAFStoreRef<Type>> paramTypes;
          paramTypes.reserve(funcType->getNumParams());
          for (auto paramType : funcType->params()) {
            paramTypes.push_back(AddLLVMType(paramType));
          }

          FunctionSignature ctorSignature { CAFStoreRef<Type> { }, std::move(paramTypes) };
          structType->AddConstructor(Constructor { std::move(ctorSignature), ctorId });
        }

        return structType;
      }
    }

    CAFStoreRef<Type> AddLLVMStructTypeAsAggregateType(
        const llvm::StructType* type, uint64_t typeId) {
      CAFStoreRef<AggregateType> aggregateType;
      if (type->isLiteral() || !type->hasName()) {
        aggregateType = _store->CreateUnnamedAggregateType(typeId);
      } else {
        auto name = type->getStructName().str();
        aggregateType = _store->CreateAggregateType(std::move(name), typeId);
      }
      _types.emplace(type, aggregateType);

      for (auto fieldType : type->elements()) {
        aggregateType->AddField(AddLLVMType(fieldType));
      }

      return aggregateType;
    }

    CAFStoreRef<Type> AddLLVMFunctionType(const llvm::FunctionType* type, uint64_t typeId) {
      auto signatureLLVM = LLVMFunctionSignature::FromType(type);
      auto i = _funcTypeIds.find(signatureLLVM);
      uint64_t signatureId;
      if (i != _funcTypeIds.end()) {
        signatureId = i->second;
      } else {
        signatureId = _funcSignatureIdAlloc.next();
        _funcTypeIds.emplace(signatureLLVM, signatureId);
      }

      auto funcType = _store->CreateFunctionType(signatureId, typeId);
      _types.emplace(type, funcType);

      auto retType = AddLLVMType(signatureLLVM.retType());
      std::vector<CAFStoreRef<Type>> paramTypes;
      for (auto t : signatureLLVM.paramTypes()) {
        paramTypes.push_back(AddLLVMType(t));
      }

      FunctionSignature cafSignature { retType, std::move(paramTypes) };
      funcType->SetSigature(std::move(cafSignature));

      // Add all callback functions that match the signature to the store.
      for (auto callbackFuncId : _frozenContext->GetCallbackFunctions(signatureLLVM)) {
        _store->AddCallbackFunction(signatureId, callbackFuncId);
      }

      return funcType;
    }
  }; // class CreateCAFStoreContext

  IncrementIdAllocator<uint64_t> _apiIdAlloc;
  IncrementIdAllocator<uint64_t> _ctorIdAlloc;
  IncrementIdAllocator<uint64_t> _typeIdAlloc;

  std::vector<std::pair<const llvm::Function *, uint64_t>> _apis;
  std::unordered_map<const llvm::Function *, uint64_t> _apiToId;
  std::unordered_map<uint64_t, const llvm::Function *> _idToApi;

  std::vector<std::pair<const llvm::Function *, uint64_t>> _ctors;
  std::unordered_map<const llvm::Function *, uint64_t> _ctorToId;
  std::unordered_map<uint64_t, const llvm::Function *> _idToCtor;

  std::vector<std::pair<const llvm::Type *, uint64_t>> _types;
  std::unordered_map<const llvm::Type *, uint64_t> _typeToId;
  std::unordered_map<uint64_t, const llvm::Type *> _idToType;

  std::unordered_multimap<const llvm::Type *, const llvm::Function *> _typeToCtors;
  std::unordered_map<const llvm::Function *, const llvm::Type *> _ctorToType;

  std::unordered_set<LLVMFunctionSignature, Hasher<LLVMFunctionSignature>>
      _callbackSignatures;
  std::vector<Either<const llvm::Function *, LLVMFunctionSignature>> _callbackFuncs;
  std::unordered_multimap<LLVMFunctionSignature, size_t, Hasher<LLVMFunctionSignature>>
      _callbackSignaturesToFuncId;
}; // class ExtractorContext::FrozenContext

ExtractorContext::ExtractorContext() = default;

ExtractorContext::ExtractorContext(ExtractorContext &&) noexcept = default;
ExtractorContext& ExtractorContext::operator=(ExtractorContext &&) = default;

ExtractorContext::~ExtractorContext() = default;

void ExtractorContext::EnsureFrozen() const {
  assert(static_cast<bool>(_frozen) && "The ExtractorContext object has not been frozen.");
}

void ExtractorContext::EnsureNotFrozen() const {
  assert(!static_cast<bool>(_frozen) && "The ExtractorContext object has been frozen.");
}

void ExtractorContext::AddApiFunction(const llvm::Function* func) {
  EnsureNotFrozen();
  _apis.push_back(func);
}

void ExtractorContext::AddConstructor(const llvm::Type* type, const llvm::Function *ctor) {
  EnsureNotFrozen();
  _ctors.emplace(type, ctor);
}

void ExtractorContext::AddCallbackFunctionCandidate(const llvm::Function* func) {
  EnsureNotFrozen();
  _callbackFunctions.push_back(func);
}

void ExtractorContext::Freeze() {
  EnsureNotFrozen();
  _frozen = caf::make_unique<FrozenContext>();

  for (auto apiFunc : _apis) {
    FreezeApiFunction(apiFunc);
  }

  for (auto callbackFunc : _callbackFunctions) {
    _frozen->AddCallbackFunctionCandidate(callbackFunc);
  }

  _frozen->PopulateLackingCallbackFunction();
}

void ExtractorContext::FreezeApiFunction(const llvm::Function* func) const {
  _frozen->AddApiFunction(func);

  auto funcType = func->getFunctionType();
  FreezeType(funcType->getReturnType());
  for (auto paramType : funcType->params()) {
    FreezeType(paramType);
  }
}

void ExtractorContext::FreezeType(const llvm::Type* type) const {
  if (_frozen->HasType(type)) {
    return;
  }
  if (!IsValidType(type)) {
    llvm::errs() << "CAF: Unrecognized type found:";
    type->dump();
    abort();
  }

  _frozen->AddType(type);
  if (type->isPointerTy()) {
    FreezeType(type->getPointerElementType());
  } else if (type->isArrayTy()) {
    FreezeType(type->getArrayElementType());
  } else if (type->isStructTy()) {
    auto structType = caf::dyn_cast<llvm::StructType>(type);
    bool isAggregate = false;
    if (structType->isLiteral() || !structType->hasName()) {
      isAggregate = true;
    } else {
      isAggregate = (_ctors.find(type) == _ctors.end());
    }

    llvm::errs() << "CAF: Freezing struct type: ";
    structType->dump();
    llvm::errs() << "\n";

    if (isAggregate) {
      for (auto field : structType->elements()) {
        FreezeType(field);
      }
    } else {
      auto r = _ctors.equal_range(type);
      for (auto i = r.first; i != r.second; ++i) {
        FreezeConstructor(type, i->second);
      }
    }
  } else if (type->isFunctionTy()) {
    auto funcType = caf::dyn_cast<llvm::FunctionType>(type);
    FreezeType(funcType->getReturnType());
    for (auto paramType : funcType->params()) {
      FreezeType(paramType);
    }
  }
}

void ExtractorContext::FreezeConstructor(const llvm::Type* type, const llvm::Function* ctor) const {
  _frozen->AddConstructor(type, ctor);

  auto ctorFuncType = ctor->getFunctionType();
  FreezeType(ctorFuncType->getReturnType());
  for (auto paramType : ctorFuncType->params()) {
    FreezeType(paramType);
  }
}

std::unique_ptr<CAFStore> ExtractorContext::CreateCAFStore() const {
  EnsureFrozen();
  return _frozen->CreateCAFStore();
}

Optional<uint64_t> ExtractorContext::GetApiFunctionId(const llvm::Function* func) const {
  EnsureFrozen();
  return _frozen->GetApiFunctionId(func);
}

Optional<uint64_t> ExtractorContext::GetConstructorId(const llvm::Function* ctor) const {
  EnsureFrozen();
  return _frozen->GetConstructorId(ctor);
}

Optional<uint64_t> ExtractorContext::GetTypeId(const llvm::Type* type) const {
  EnsureFrozen();
  return _frozen->GetTypeId(type);
}

Optional<const llvm::Function *> ExtractorContext::GetApiFunctionById(uint64_t id) const {
  EnsureFrozen();
  return _frozen->GetApiFunctionById(id);
}

std::vector<const llvm::Function *> ExtractorContext::GetApiFunctions() const {
  EnsureFrozen();
  return _frozen->GetApiFunctions();
}

size_t ExtractorContext::GetApiFunctionsCount() const {
  EnsureFrozen();
  return _frozen->GetApiFunctionsCount();
}

Optional<const llvm::Function *> ExtractorContext::GetConstructorById(uint64_t id) const {
  EnsureFrozen();
  return _frozen->GetConstructorById(id);
}

Optional<const llvm::Type *> ExtractorContext::GetTypeById(uint64_t id) const {
  EnsureFrozen();
  return _frozen->GetTypeById(id);
}

size_t ExtractorContext::GetTypesCount() const {
  EnsureFrozen();
  return _frozen->GetTypesCount();
}

std::vector<const llvm::Function *> ExtractorContext::GetConstructorsOfType(
    const llvm::Type *type) const {
  EnsureFrozen();
  return _frozen->GetConstructorsOfType(type);
}

Optional<const llvm::Type *> ExtractorContext::GetConstructingType(
    const llvm::Function *ctor) const {
  EnsureFrozen();
  return _frozen->GetConstructingType(ctor);
}

std::vector<const llvm::Function *> ExtractorContext::GetConstructors() const {
  EnsureFrozen();
  return _frozen->GetConstructors();
}

size_t ExtractorContext::GetConstructorsCount() const {
  EnsureFrozen();
  return _frozen->GetConstructorsCount();
}

const std::vector<Either<const llvm::Function *, LLVMFunctionSignature>> &
ExtractorContext::GetCallbackFunctionCandidates() const {
  EnsureFrozen();
  return _frozen->GetCallbackFunctions();
}

size_t ExtractorContext::GetCallbackFunctionsCount() const {
  EnsureFrozen();
  return _frozen->GetCallbackFunctionsCount();
}

} // namespace caf
