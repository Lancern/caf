#ifndef CAF_SYMBOL_TABLE_H
#define CAF_SYMBOL_TABLE_H

#include "FunctionSignatureGrouper.h"

#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace llvm {
class Function;
class Type;
} // namespace llvm

namespace caf {

class CAFStore;
template <typename T> class CAFStoreRef;
class Constructor;

class Type;
class Function;

namespace {
class SymbolTableFreezeContext;
} // namespace <anonymous>

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
  void clear();

  /**
   * @brief Add the given function to the symbol table as an API definition.
   *
   * If the given function already exists in this symbol table, then the given function will be not
   * added again into the symbol table.
   *
   * @param func the function to be added.
   */
  void AddApi(llvm::Function* func);

  /**
   * @brief Add the given function to the symbol table as a constructor of the given type.
   *
   * The duplication of the given function is not checked.
   *
   * @param typeName the identifier of the type.
   * @param func the function to be added.
   */
  void AddConstructor(const std::string& typeName, llvm::Function* func);

  /**
   * @brief Add a callback function candidate. A callback function candidate is a function that can
   * be selected as the pointee of some function pointer.
   *
   * @param candidate the candidate to add.
   */
  void AddCallbackFunctionCandidate(llvm::Function* candidate);

  /**
   * @brief Get the list of API definitions contained in the symbol table.
   *
   * @return const std::vector<llvm::Function *>& list of API definitions contained in the symbol
   * table.
   */
  const std::vector<llvm::Function *>& apis() const { return _apis; }

  /**
   * @brief Get a list of constructors contained in the symbol table.
   *
   * @param typeName the identifier of the desired type.
   * @return const std::vector<llvm::Function *>* pointer to the list of constructors of the given
   * type, or nullptr if the identifier cannot be found.
   */
  const std::vector<llvm::Function *>* GetConstructors(const std::string& typeName) const;

  /**
   * @brief Create a CAFStore instance holding CAF representation of the symbols
   * in this symbol table.
   *
   * @return std::unique_ptr<CAFStore> the created CAFStore object.
   */
  std::unique_ptr<CAFStore> GetCAFStore() const;

private:
  // Fuzz-target APIs.
  std::vector<llvm::Function *> _apis;
  std::unordered_set<std::string> _apiNames;

  // List of constructors.
  std::unordered_map<std::string, std::vector<llvm::Function *>> _ctors;

  // Groups callback function candidates by their function signatures.
  FunctionSignatureGrouper _callbackFunctionGrouper;

  /**
   * @brief Create an instance of @see Constructor from the given LLVM function.
   *
   * @param context the current freeze context.
   * @param func the LLVM function.
   * @param constructingType the type the constructor constructs.
   * @return Constructor the created @see Constructor instance.
   */
  Constructor CreateConstructorFromLLVMFunction(
      SymbolTableFreezeContext& context,
      const llvm::Function* func,
      CAFStoreRef<Type> constructingType) const;

  /**
   * @brief Add the given LLVM type definition to the CAFStore contained in the given freeze
   * context. The corresponding constructors (if the given type is a struct type) and all reachable
   * types will be added to the CAFStore recursively.
   *
   * @param context the freeze context.
   * @param type the type to add.
   * @return CAFStoreRef<Type> pointer to the added CAF type definition.
   */
  CAFStoreRef<Type> AddLLVMTypeToStore(
      SymbolTableFreezeContext& context, const llvm::Type* type) const;

  /**
   * @brief Add the given LLVM function to the CAFStore contained in the given freeze context as an
   * API definition. The types reachable from the given function will be added to the store
   * recursively. Construction of type definitions will use the constructors defined in this symbol
   * table.
   *
   * @param context the freeze context.
   * @param func the function to be added.
   * @return CAFStoreRef<Function> pointer to the added CAF function definition.
   */
  CAFStoreRef<Function> AddLLVMApiFunctionToStore(
      SymbolTableFreezeContext& context, const llvm::Function* func) const;
};

} // namespace caf

#endif
