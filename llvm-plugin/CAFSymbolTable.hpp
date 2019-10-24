#ifndef CAF_SYMBOL_TABLE_HPP
#define CAF_SYMBOL_TABLE_HPP

#include <vector>
#include <string>
#include <unordered_map>

#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Function.h"

#include "CAFMeta.hpp"


namespace {

/**
 * @brief Get the name of the given LLVM type.
 * 
 * @param type the LLVM type.
 * @return std::string the name of the given LLVM type.
 */
inline std::string getLLVMTypeName(const llvm::Type* type) noexcept {
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
inline size_t getLLVMTypeSize(const llvm::Type* type) noexcept {
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
 * @brief Symbol table definition used in CAF.
 * 
 */
class CAFSymbolTable {
public:
  /**
   * @brief Clear all symbols defined in this symbol table.
   * 
   */
  void clear() noexcept {
    _apis.clear();
    _ctors.clear();
    _factories.clear();
  }

  /**
   * @brief Add the given function to the symbol table as an API definition.
   * 
   * @param func the function to be added.
   */
  void addApi(llvm::Function* func) noexcept {
    _apis.push_back(func);
  }

  /**
   * @brief Add the given function to the symbol table as a constructor of the
   * given type.
   * 
   * @param typeName the identifier of the type.
   * @param func the function to be added.
   */
  void addConstructor(const std::string& typeName, llvm::Function* func) 
      noexcept {
    _ctors[typeName].push_back(func);
  }

  /**
   * @brief Add the given function to the symbol table as a factory function of
   * the given type.
   * 
   * @param typeName the identifier of the type.
   * @param func the function to be added.
   */
  void addFactoryFunction(
      const std::string& typeName, llvm::Function* func) noexcept {
    _factories[typeName].push_back(func);
  }

  /**
   * @brief Get the list of API definitions contained in the symbol table.
   * 
   * @return const std::vector<llvm::Function *>& list of API definitions
   * contained in the symbol table.
   */
  const std::vector<llvm::Function *>& apis() const noexcept {
    return _apis;
  }

  /**
   * @brief Get a list of constructors contained in the symbol table.
   * 
   * @param typeName the identifier of the desired type.
   * @return const std::vector<llvm::Function *>* pointer to the list of
   * constructors of the given type, or nullptr if the identifier cannot be
   * found.
   */
  const std::vector<llvm::Function *>* getConstructor(
      const std::string& typeName) const noexcept {
    auto i = _ctors.find(typeName);
    if (i == _ctors.end()) {
      return nullptr;
    } else {
      return &i->second;
    }
  }

  /**
   * @brief Get a list of factory functions contained in the symbol table.
   * 
   * @param typeName the identifier of the desired type.
   * @return const std::vector<llvm::Function *>* pointer to the list of
   * factory functions of the given type, or nullptr if the identifier cannot
   * be found.
   */
  const std::vector<llvm::Function *>* getFactoryFunction(
      const std::string& typeName) const noexcept {
    auto i = _factories.find(typeName);
    if (i == _factories.end()) {
      return nullptr;
    } else {
      return &i->second;
    }
  }

  /**
   * @brief Create a CAFStore instance holding CAF representation of the symbols
   * in this symbol table.
   * 
   * @return std::unique_ptr<caf::CAFStore> the created CAFStore object.
   */
  std::unique_ptr<caf::CAFStore> getCAFStore() const noexcept {
    auto store = std::make_unique<caf::CAFStore>();
    
    for (const auto& func : _apis) {
      addLLVMFunctionToStore(func, *store);
    }

    return store;
  }

private:
  // Top-level APIs.
  std::vector<llvm::Function *> _apis;

  // List of constructors.
  std::unordered_map<std::string, std::vector<llvm::Function *>> _ctors;

  // List of factory functions, a.k.a. functions whose argument list is empty
  // and returns an instance of some type.
  std::unordered_map<std::string, std::vector<llvm::Function *>> _factories;

  /**
   * @brief Create an instance of caf::Activator from the given LLVM function.
   * 
   * @param func the LLVM function.
   * @param kind the kind of activator.
   * @param constructingType the type the activator constructs.
   * @return std::unique_ptr<caf::Activator> the created caf::Activator 
   * instance.
   */
  std::unique_ptr<caf::Activator> createActivatorFromLLVMFunction(
      const llvm::Function* func, 
      caf::ActivatorKind kind,
      caf::CAFStoreRef<caf::Type> constructingType) const noexcept {
    auto& store = *constructingType.store();

    std::vector<caf::CAFStoreRef<caf::Type>> args { };
    for (const auto& a : func->args()) {
      args.push_back(addLLVMTypeToStore(a.getType(), store));
    }
    auto returnType = addLLVMTypeToStore(func->getReturnType(), store);

    caf::FunctionSignature signature { returnType, std::move(args) };
    return std::make_unique<caf::Activator>(
        &store, constructingType, kind, func->getName().str(), 
        std::move(signature));
  }

  /**
   * @brief Add the given LLVM type definition to the given CAFStore. The
   * corresponding activators (constructors and factory functions) and all
   * reachable types will be added to the CAFStore recursively.
   * 
   * @param type the type to add.
   * @param store the store.
   * @return caf::CAFStoreRef<caf::Type> pointer to the added CAF type 
   * definition.
   */
  caf::CAFStoreRef<caf::Type> addLLVMTypeToStore(
      const llvm::Type* type, caf::CAFStore& store) const noexcept {
    if (type->isVoidTy()) {
      // Returns an empty CAFStoreRef instance to represent a void type.
      return caf::CAFStoreRef<caf::Type> { };
    } else if (type->isIntegerTy() || type->isFloatingPointTy()) {
      // Add a BitsType instance to the store.
      auto typeName = getLLVMTypeName(type);
      auto typeSize = getLLVMTypeSize(type);
      return store.createBitsType(std::move(typeName), typeSize);
    } else if (type->isPointerTy()) {
      // Add a PointerType instance to the store.
      auto pointee = addLLVMTypeToStore(type->getPointerElementType(), store);
      return store.createPointerType(pointee);
    } else if (type->isStructTy()) {
      // Add a StructType instance to the store.
      // Is there already a struct with the same name in the store? If so, we
      // return that struct definition directly.
      auto structType = llvm::dyn_cast<llvm::StructType>(type);
      if (structType->hasName()) {
        auto name = structType->getName().str();
        if (store.containsType(name)) {
          return store.getType(name);
        }
      }

      auto cafStructType = structType->hasName()
          ? store.createStructType(structType->getName().str())
          : store.createUnnamedStructType();
      
      for (auto subtype : structType->elements()) {
        cafStructType->addField(addLLVMTypeToStore(subtype, store));
      }

      if (structType->hasName()) {
        // Attach all activators to this struct type.
        auto name = structType->getName().str();

        auto ctors = _ctors.find(name);
        if (ctors != _ctors.end()) {
          for (auto c : ctors->second) {
            cafStructType->addActivator(createActivatorFromLLVMFunction(
                c, caf::ActivatorKind::Constructor, cafStructType));
          }
        }

        auto factories = _factories.find(name);
        if (factories != _factories.end()) {
          for (auto f : factories->second) {
            cafStructType->addActivator(createActivatorFromLLVMFunction(
                f, caf::ActivatorKind::Factory, cafStructType));
          }
        }
      }

      return cafStructType;
    } else {
      // Unrecognizable type. Returns an empty CAFStoreRef instance to
      // represent it.
      llvm::errs() 
          << "CAF: warning: trying to add unrecognized LLVM type to CAF store:"
          << type->getTypeID()
          << "\n";
      return caf::CAFStoreRef<caf::Type> { };
    }
  }

  /**
   * @brief Add the given LLVM function to the given CAFStore as an API
   * definition. The types reachable from the given function will be added
   * to the store recursively. Construction of type definitions will use the 
   * constructors and factory functions defined in this symbol table.
   * 
   * @param func the function to be added.
   * @param store the CAFStore.
   * @return caf::CAFStoreRef<caf::Function> pointer to the added CAF function
   * definition.
   */
  caf::CAFStoreRef<caf::Function> addLLVMFunctionToStore(
      const llvm::Function* func, caf::CAFStore& store) const noexcept {
    // Add the types of arguments and return value to the store.
    std::vector<caf::CAFStoreRef<caf::Type>> argTypes { };
    for (const auto& arg : func->args()) {
      argTypes.push_back(addLLVMTypeToStore(arg.getType(), store));
    }
    auto returnType = addLLVMTypeToStore(func->getReturnType(), store);

    auto funcName = func->hasName() 
        ? func->getName().str() 
        : std::string("");
    caf::FunctionSignature signature { returnType, std::move(argTypes) };
    
    return store.createApi(std::move(funcName), std::move(signature));
  }
};

} // namespace <anonymous>


#endif
