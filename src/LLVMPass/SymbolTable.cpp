#include "SymbolTable.h"
#include "Infrastructure/Memory.h"
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

} // namespace <anonymous>

void CAFSymbolTable::clear() {
  _apis.clear();
  _apiNames.clear();
  _ctors.clear();
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

void CAFSymbolTable::AddCallbackFunction(llvm::Function* func) {
  _callbacks.push_back(func);
}

const std::vector<llvm::Function *>* CAFSymbolTable::GetConstructors(const std::string& typeName)
    const {
  auto i = _ctors.find(typeName);
  if (i == _ctors.end()) {
    return nullptr;
  } else {
    return &i->second;
  }
}

std::unique_ptr<CAFStore> CAFSymbolTable::GetCAFStore() const {
  auto store = caf::make_unique<CAFStore>();

  for (const auto& func : _apis) {
    AddLLVMApiFunctionToStore(func, *store);
  }

  return store;
}

Constructor CAFSymbolTable::CreateConstructorFromLLVMFunction(
    const llvm::Function* func, CAFStoreRef<Type> constructingType) const {
  auto& store = *constructingType.store();

  std::vector<CAFStoreRef<Type>> args { };
  for (const auto& a : func->args()) {
    args.push_back(AddLLVMTypeToStore(a.getType(), store));
  }
  auto returnType = AddLLVMTypeToStore(func->getReturnType(), store);

  FunctionSignature signature { returnType, std::move(args) };
  return Constructor { std::move(signature) };
}

CAFStoreRef<Type> CAFSymbolTable::AddLLVMTypeToStore(
    const llvm::Type* type, CAFStore& store) const {
  if (type->isVoidTy()) {
    // Returns an empty CAFStoreRef instance to represent a void type.
    return CAFStoreRef<Type> { };
  } else if (type->isIntegerTy() || type->isFloatingPointTy()) {
    // Add a BitsType instance to the store.
    auto typeName = GetLLVMTypeName(type);
    auto typeSize = GetLLVMTypeSize(type);
    return store.CreateBitsType(std::move(typeName), typeSize);
  } else if (type->isPointerTy()) {
    // Add a PointerType instance to the store.
    auto pointee = AddLLVMTypeToStore(type->getPointerElementType(), store);
    return store.CreatePointerType(pointee);
  } else if (type->isArrayTy()) {
    // Add an ArrayType instance to the store.
    auto element = AddLLVMTypeToStore(type->getArrayElementType(), store);
    return store.CreateArrayType(type->getArrayNumElements(), element);
  } else if (type->isStructTy()) {
    // Add a StructType instance to the store.
    // Is there already a struct with the same name in the store? If so, we return that struct
    // definition directly.
    auto structType = llvm::dyn_cast<llvm::StructType>(type);
    if (structType->hasName()) {
      auto name = structType->getName().str();
      if (store.ContainsType(name)) {
        return store.GetType(name);
      }
    }

    auto cafStructType = structType->hasName()
        ? store.CreateStructType(structType->getName().str())
        : store.CreateUnnamedStructType();

    if (structType->hasName()) {
      // Attach all activators to this struct type.
      auto name = structType->getName().str();

      auto ctors = _ctors.find(name);
      if (ctors != _ctors.end()) {
        for (auto c : ctors->second) {
          cafStructType->AddConstructor(CreateConstructorFromLLVMFunction(c, cafStructType));
        }
      }
    }

    return cafStructType;
  } else if (type->isFunctionTy()) {
    // Add a FunctionType instance to the store.
    auto funcTypeLLVM = llvm::dyn_cast<llvm::FunctionType>(type);

    // Add the return type and argument types to the store.
    auto retType = AddLLVMTypeToStore(funcTypeLLVM->getReturnType(), store);
    std::vector<CAFStoreRef<Type>> argTypes { };
    argTypes.reserve(funcTypeLLVM->getNumParams());
    for (auto argTypeLLVM : funcTypeLLVM->params()) {
      argTypes.push_back(AddLLVMTypeToStore(argTypeLLVM, store));
    }

    // Construct a new FunctionSignature that represents the signature of the current function type.
    FunctionSignature signature { retType, std::move(argTypes) };
    return store.CreateFunctionType(std::move(signature));
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
    const llvm::Function* func, CAFStore& store) const {
  // Add the types of arguments and return value to the store.
  std::vector<CAFStoreRef<Type>> argTypes { };
  for (const auto& arg : func->args()) {
    argTypes.push_back(AddLLVMTypeToStore(arg.getType(), store));
  }
  auto returnType = AddLLVMTypeToStore(func->getReturnType(), store);

  auto funcName = func->hasName()
      ? func->getName().str()
      : std::string("");
  FunctionSignature signature { returnType, std::move(argTypes) };

  return store.CreateApi(std::move(funcName), std::move(signature));
}

} // namespace caf
