#ifndef CAF_SYMBOL_TABLE_H
#define CAF_SYMBOL_TABLE_H

#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace llvm {
class Function;
} // namespace llvm

namespace caf {

class CAFStore;
template <typename T> class CAFStoreRef;
class Constructor;

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
  void AddConstructor(const std::string& typeName, llvm::Function* func) {
    _ctors[typeName].push_back(func);
  }

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

  /**
   * @brief Create an instance of @see Constructor from the given LLVM function.
   *
   * @param func the LLVM function.
   * @param constructingType the type the constructor constructs.
   * @return Constructor the created @see Constructor instance.
   */
  Constructor CreateConstructorFromLLVMFunction(
      const llvm::Function* func,
      CAFStoreRef<Type> constructingType) const;

  /**
   * @brief Add the given LLVM type definition to the given CAFStore. The corresponding activators
   * (constructors and factory functions) and all reachable types will be added to the CAFStore
   * recursively.
   *
   * @param type the type to add.
   * @param store the store.
   * @return CAFStoreRef<Type> pointer to the added CAF type definition.
   */
  CAFStoreRef<Type> AddLLVMTypeToStore(const llvm::Type* type, CAFStore& store) const;

  /**
   * @brief Add the given LLVM function to the given CAFStore as an API
   * definition. The types reachable from the given function will be added
   * to the store recursively. Construction of type definitions will use the
   * constructors and factory functions defined in this symbol table.
   *
   * @param func the function to be added.
   * @param store the CAFStore.
   * @return CAFStoreRef<Function> pointer to the added CAF function
   * definition.
   */
  CAFStoreRef<Function> AddLLVMFunctionToStore(const llvm::Function* func, CAFStore& store) const;
};

} // namespace caf

#endif
