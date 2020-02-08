#include "SymbolTable.h"
#include "Infrastructure/Memory.h"
#include "Infrastructure/Hash.h"
#include "Basic/Identity.h"
#include "Basic/CAFStore.h"
#include "Basic/Type.h"
#include "Basic/BitsType.h"
#include "Basic/PointerType.h"
#include "Basic/ArrayType.h"
#include "Basic/StructType.h"
#include "Basic/FunctionType.h"
#include "Basic/Constructor.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Function.h"

#include <utility>

namespace caf {

namespace {

/**
 * @brief Get the name of the given LLVM type.
 *
 * @param type the LLVM type.
 * @return std::string the name of the given LLVM type.
 */
std::string GetLLVMTypeName(const llvm::Type* type) noexcept {
  switch (type->getTypeID()) {
    case llvm::Type::VoidTyID:
      return "void";
    case llvm::Type::HalfTyID:
      return "f16";
    case llvm::Type::FloatTyID:
      return "f32";
    case llvm::Type::DoubleTyID:
      return "f64";
    case llvm::Type::X86_FP80TyID:
      return "f80";
    case llvm::Type::FP128TyID:
      return "f128";
    case llvm::Type::PPC_FP128TyID:
      return "f128ppc";
    case llvm::Type::IntegerTyID: {
      std::string name("i");
      name.append(std::to_string(type->getIntegerBitWidth()));
      return name;
    }
    case llvm::Type::StructTyID:
      return type->getStructName().str();
    default:
      return "";
  }
}

/**
 * @brief Get the size of the LLVM type, in bytes.
 *
 * @param type the LLVM type.
 * @return size_t size of the LLVM type, in bytes.
 */
size_t GetLLVMTypeSize(const llvm::Type* type) noexcept {
  switch (type->getTypeID()) {
    case llvm::Type::HalfTyID:
      return 2;
    case llvm::Type::FloatTyID:
      return 4;
    case llvm::Type::DoubleTyID:
      return 8;
    case llvm::Type::X86_FP80TyID:
      return 10;
    case llvm::Type::FP128TyID:
      return 16;
    case llvm::Type::PPC_FP128TyID:
      return 16;
    case llvm::Type::IntegerTyID:
      return type->getIntegerBitWidth() / 8;
    default:
      return 0;
  }
}

/**
 * @brief Provide context when converting @see CAFSymbolTable object to @see CAFStore object.
 *
 */
class SymbolTableFreezeContext {
public:
  /**
   * @brief Construct a new @see SymbolTableFreezeContext object.
   *
   * @param store the underlying @see CAFStore object being initializing.
   */
  explicit SymbolTableFreezeContext(std::unique_ptr<CAFStore> store)
    : _store(std::move(store))
  { }

  /**
   * @brief Get a pointer to the @see CAFStore object being initializing.
   *
   * @return CAFStore* a pointer to the @see CAFStore object being initializing.
   */
  CAFStore* store() const { return _store.get(); }

  /**
   * @brief Takes ownership of the underlying @see CAFStore object.
   *
   * After calling this method, this @see SymbolTableFreezeContext object should not be used again
   * or the behavior is undefined.
   *
   * @return std::unique_ptr<CAFStore> an owned pointer to the initialized @see CAFStore object.
   */
  std::unique_ptr<CAFStore> TakeStore() { return std::move(_store); }

  /**
   * @brief Takes ownership of the underlying callback function candidate list.
   *
   * @return std::vector<Either<llvm::Function *, LLVMFunctionSignature>> a list of registered
   * callback function candidates.
   */
  std::vector<Either<llvm::Function *, LLVMFunctionSignature>> TakeCallbackFunctions() {
    return std::move(_callbackFunctions);
  }

  /**
   * @brief Get the ID of the given LLVM function signature.
   *
   * This function returns a std::pair<bool, uint64_t> whose:
   * * `first` field indicate whether the signature has been seen before;
   * * `second` field is the ID of the given signature.
   *
   * If the given signature has not been seen before, a new signature ID will be allocated and
   * returned.
   *
   * @param signature
   * @return std::pair<bool, uint64_t>
   */
  std::pair<bool, uint64_t> GetSignatureId(LLVMFunctionSignature signature) {
    auto i = _signatureIds.find(signature);
    if (i == _signatureIds.end()) {
      auto id = _signatureIdAlloc.next();
      _signatureIds.emplace(std::move(signature), id);
      return std::make_pair(false, id);
    }
    return std::make_pair(true, i->second);
  }

  /**
   * @brief Add the given function to the callback function candidate list.
   *
   * @param function the function to add.
   * @return size_t the ID of the added callback function.
   */
  size_t AddCallbackFunction(llvm::Function* function) {
    return AddCallbackFunction(
        Either<llvm::Function *, LLVMFunctionSignature>::CreateLhs(function));
  }

  /**
   * @brief Add the given function signature to the callback function candidate list.
   *
   * @param signature the function signature to add.
   * @return size_t teh ID of the added callback function.
   */
  size_t AddCallbackFunction(LLVMFunctionSignature signature) {
    return AddCallbackFunction(
        Either<llvm::Function *, LLVMFunctionSignature>::CreateRhs(signature));
  }

private:
  std::unique_ptr<CAFStore> _store;
  std::unordered_map<LLVMFunctionSignature, uint64_t, Hasher<LLVMFunctionSignature>> _signatureIds;
  std::vector<Either<llvm::Function *, LLVMFunctionSignature>> _callbackFunctions;
  IncrementIdAllocator<uint64_t> _signatureIdAlloc;

  /**
   * @brief Add the given callback function representation to the callback function candidate list.
   *
   * @param fn the function representation to add.
   * @return size_t the ID of the added callback function.
   */
  size_t AddCallbackFunction(Either<llvm::Function *, LLVMFunctionSignature> fn) {
    auto id = _callbackFunctions.size();
    _callbackFunctions.push_back(std::move(fn));
    return id;
  }
}; // class SymbolTableFreezeContext

} // namespace <anonymous>

void CAFSymbolTable::clear() {
  _apis.clear();
  _apiNames.clear();
  _ctors.clear();
  _callbackFunctionGrouper.clear();
}

void CAFSymbolTable::AddApi(llvm::Function* func) {
  auto funcName = func->getName().str();
  if (_apiNames.find(funcName) != _apiNames.end()) {
    return;
  }

  _apis.push_back(func);
  _apiNames.insert(std::move(funcName));
}

void CAFSymbolTable::AddConstructor(const std::string& typeName, llvm::Function* func) {
  _ctors[typeName].push_back(func);
}

void CAFSymbolTable::AddCallbackFunctionCandidate(llvm::Function* candidate) {
  _callbackFunctionGrouper.Register(candidate);
}

const std::vector<llvm::Function *>* CAFSymbolTable::GetConstructors(
    const std::string& typeName) const {
  auto i = _ctors.find(typeName);
  if (i == _ctors.end()) {
    return nullptr;
  } else {
    return &i->second;
  }
}

CAFSymbolTableFreezeResult CAFSymbolTable::Freeze() const {
  auto store = caf::make_unique<CAFStore>();
  SymbolTableFreezeContext context { std::move(store) };

  for (auto func : _apis) {
    AddLLVMApiFunctionToStore(context, func);
  }

  CAFSymbolTableFreezeResult result;
  result.store = context.TakeStore();
  result.callbackFunctions = context.TakeCallbackFunctions();
  return result;
}

Constructor CAFSymbolTable::CreateConstructorFromLLVMFunction(
    SymbolTableFreezeContext& context,
    const llvm::Function* func,
    CAFStoreRef<Type> constructingType) const {
  std::vector<CAFStoreRef<Type>> args { };
  for (const auto& a : func->args()) {
    args.push_back(AddLLVMTypeToStore(context, a.getType()));
  }
  auto returnType = AddLLVMTypeToStore(context, func->getReturnType());

  FunctionSignature signature { returnType, std::move(args) };
  return Constructor { std::move(signature) };
}

CAFStoreRef<Type> CAFSymbolTable::AddLLVMTypeToStore(
    SymbolTableFreezeContext& context, const llvm::Type* type) const {
  assert(type && "Trying to add a nullptr of llvm::Type * to store.");

  auto store = context.store();
  if (type->isVoidTy()) {
    // Returns an empty CAFStoreRef instance to represent a void type.
    return CAFStoreRef<Type> { };
  } else if (type->isIntegerTy() || type->isFloatingPointTy()) {
    // Add a BitsType instance to the store.
    auto typeName = GetLLVMTypeName(type);
    auto typeSize = GetLLVMTypeSize(type);
    return store->CreateBitsType(std::move(typeName), typeSize);
  } else if (type->isPointerTy()) {
    // Add a PointerType instance to the store.
    auto pointee = AddLLVMTypeToStore(context, type->getPointerElementType());
    return store->CreatePointerType(pointee);
  } else if (type->isArrayTy()) {
    // Add an ArrayType instance to the store.
    auto element = AddLLVMTypeToStore(context, type->getArrayElementType());
    return store->CreateArrayType(type->getArrayNumElements(), element);
  } else if (type->isStructTy()) {
    // Add a StructType instance to the store.
    // Is there already a struct with the same name in the store? If so, we return that struct
    // definition directly.
    auto structType = llvm::dyn_cast<llvm::StructType>(type);
    if (structType->hasName()) {
      auto name = structType->getName().str();
      if (store->ContainsType(name)) {
        return store->GetType(name);
      }
    }

    auto cafStructType = structType->hasName()
        ? store->CreateStructType(structType->getName().str())
        : store->CreateUnnamedStructType();

    if (structType->hasName()) {
      // Attach all constructors to this struct type.
      auto name = structType->getName().str();

      auto ctors = _ctors.find(name);
      if (ctors != _ctors.end()) {
        for (auto c : ctors->second) {
          cafStructType->AddConstructor(CreateConstructorFromLLVMFunction(
              context, c, cafStructType));
        }
      }
    }

    return cafStructType;
  } else if (type->isFunctionTy()) {
    auto signature = LLVMFunctionSignature::FromType(type);
    auto existId = context.GetSignatureId(signature);
    if (existId.first) {
      // The same function signature already exists.
      return context.store()->GetFunctionType(existId.second);
    }

    // Note that funcType need to be added to the store before recurse down to add return type and
    // parameter types of the function signature.
    auto funcType = context.store()->CreateFunctionType(existId.second);

    auto retType = AddLLVMTypeToStore(context, signature.retType());
    std::vector<CAFStoreRef<Type>> paramTypes;
    for (auto t : signature.paramTypes()) {
      paramTypes.push_back(AddLLVMTypeToStore(context, t));
    }

    FunctionSignature cafSignature { retType, std::move(paramTypes) };
    funcType->SetSigature(std::move(cafSignature));

    // Add callback function candidates that match the current function signature to the store.
    auto callbackFunctions = _callbackFunctionGrouper.GetFunctions(signature);
    if (callbackFunctions) {
      for (auto fn : *callbackFunctions) {
        auto id = context.AddCallbackFunction(fn);
        context.store()->AddCallbackFunction(existId.second, id);
      }
    } else {
      // No existing functions match the signature. Add the function signature to the callback
      // function candidates list to let the code generator to generate one.
      auto id = context.AddCallbackFunction(signature);
      context.store()->AddCallbackFunction(existId.second, id);
    }

    return funcType;
  } else {
    // Unrecognizable type. Returns an empty CAFStoreRef instance to
    // represent it.
    llvm::errs()
        << "CAF: warning: trying to add unrecognized LLVM type to CAF store:"
        << type->getTypeID()
        << "\n";
    return CAFStoreRef<Type> { };
  }
}

CAFStoreRef<Function> CAFSymbolTable::AddLLVMApiFunctionToStore(
    SymbolTableFreezeContext& context, const llvm::Function* func) const {
  auto store = context.store();
  // Add the types of arguments and return value to the store.
  std::vector<CAFStoreRef<Type>> argTypes { };
  for (const auto& arg : func->args()) {
    argTypes.push_back(AddLLVMTypeToStore(context, arg.getType()));
  }
  auto returnType = AddLLVMTypeToStore(context, func->getReturnType());

  auto funcName = func->hasName()
      ? func->getName().str()
      : std::string("");
  FunctionSignature signature { returnType, std::move(argTypes) };

  return store->CreateApi(std::move(funcName), std::move(signature));
}

} // namespace caf
