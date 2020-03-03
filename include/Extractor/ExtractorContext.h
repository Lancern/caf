#ifndef CAF_EXTRACTOR_CONTEXT_H
#define CAF_EXTRACTOR_CONTEXT_H

#include "Infrastructure/Optional.h"
#include "Infrastructure/Either.h"
#include "Extractor/LLVMFunctionSignature.h"

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

namespace llvm {
class Function;
class Type;
} // namespace llvm

namespace caf {

class CAFStore;

/**
 * @brief Provide context information for CAF metadata extractor pass.
 *
 */
class ExtractorContext {
public:
  /**
   * @brief Construct a new ExtractorContext object.
   *
   */
  explicit ExtractorContext();

  ExtractorContext(const ExtractorContext &) = delete;
  ExtractorContext(ExtractorContext &&) noexcept;

  ExtractorContext& operator=(const ExtractorContext &) = delete;
  ExtractorContext& operator=(ExtractorContext &&);

  ~ExtractorContext();

  /**
   * @brief Determine whether this context has been frozen.
   *
   * @return true if this context has been frozen.
   * @return false if this context has not been frozen.
   */
  bool frozen() const { return static_cast<bool>(_frozen); }

  /**
   * @brief Add a new API function to the context.
   *
   * If the context has been frozen, this function will trigger an assertion failure.
   *
   * @param func the API function to add.
   */
  void AddApiFunction(const llvm::Function* func);

  /**
   * @brief Add a new constructor function to the context.
   *
   * If the context has been frozen, this function will trigger an assertion failure.
   *
   * @param constructingTypeName the name of the type being constructed by the given constructor.
   * @param ctor the constructor function.
   */
  void AddConstructor(const llvm::Type* type, const llvm::Function* ctor);

  /**
   * @brief Add the given function as a callback function candidate to the context.
   *
   * If the context has been frozen, this function will trigger an assertion failure.
   *
   * @param func the function to be added.
   */
  void AddCallbackFunctionCandidate(const llvm::Function* func);

  /**
   * @brief Freeze this context.
   *
   * If the context has been frozen, this function will trigger an assertion failure.
   *
   */
  void Freeze();

  /**
   * @brief Create a CAFStore object from this context.
   *
   * If the context has not been frozen, this function will trigger an assertion failure.
   *
   * @return std::unique_ptr<CAFStore> the created CAFStore object.
   */
  std::unique_ptr<CAFStore> CreateCAFStore() const;

  /**
   * @brief Get the ID of the given function.
   *
   * This function should only be called after this context has been frozen. Otherwise this function
   * will trigger an assertion failure.
   *
   * @param func the function.
   * @return Optional<uint64_t> the ID of the function. If the given function is not a registered
   * API function, returns an empty Optional object.
   */
  Optional<uint64_t> GetApiFunctionId(const llvm::Function* func) const;

  /**
   * @brief Get the ID of the given constructor.
   *
   * This function should only be called after this context has been frozen. Otherwise this function
   * will trigger an assertion failure.
   *
   * @param ctor the constructor.
   * @return Optional<uint64_t> the ID of the constructor. If the given function is not a registered
   * constructor, returns an empty Optional object.
   */
  Optional<uint64_t> GetConstructorId(const llvm::Function* ctor) const;

  /**
   * @brief Get the ID of the given type.
   *
   * This function should only be called after this context has been frozen. Otherwise this function
   * will trigger an assertion failure.
   *
   * @param type the LLVM type.
   * @return Optional<uint64_t> the ID of the type. If the given type is not a registered type,
   * returns an empty Optional object.
   */
  Optional<uint64_t> GetTypeId(const llvm::Type* type) const;

  /**
   * @brief Get the API function that has the given ID.
   *
   * This function should only be called after this context has been frozen. Otherwise this function
   * will trigger an assertion failure.
   *
   * @param id the ID of the API function.
   * @return Optional<const llvm::Function *> the API function. If the given ID does not exist,
   * returns an empty Optional object.
   */
  Optional<const llvm::Function *> GetApiFunctionById(uint64_t id) const;

  /**
   * @brief Get all registered API functions.
   *
   * This function should only be called after this context has been frozen. Otherwise this function
   * will trigger an assertion failure.
   *
   * @return std::vector<const llvm::Function *> all registered API functions.
   */
  std::vector<const llvm::Function *> GetApiFunctions() const;

  /**
   * @brief Get the number of API functions.
   *
   * This function should only be called after this context has been frozen. Otherwise this function
   * will trigger an assertion failure.
   *
   * @return size_t the number of API functions.
   */
  size_t GetApiFunctionsCount() const;

  /**
   * @brief Get the constructor that has the given ID.
   *
   * This function should only be called after this context has been frozen. Otherwise this function
   * will trigger an assertion failure.
   *
   * @param id the ID of the constructor.
   * @return Optional<const llvm::Function *> the constructor. If the given ID does not exist,
   * returns an empty Optional object.
   */
  Optional<const llvm::Function *> GetConstructorById(uint64_t id) const;

  /**
   * @brief Get the LLVM type that has the given ID.
   *
   * This function should only be called after this context has been frozen. Otherwise this function
   * will trigger an assertion failure.
   *
   * @param id the ID of the LLVM type.
   * @return Optional<const llvm::Function *> the LLVM type. If the given ID does not exist, returns
   * an empty Optional object.
   */
  Optional<const llvm::Type *> GetTypeById(uint64_t id) const;

  /**
   * @brief Get all frozen types.
   *
   * This function should only be called after this context has been frozen. Otherwise this function
   * will trigger an assertion failure.
   *
   * @return std::vector<const llvm::Type *> a list of all frozen types.
   */
  std::vector<const llvm::Type *> GetTypes() const;

  /**
   * @brief Get the type IDs of all frozen types.
   *
   * This function should only be called after this context has been frozen. Otherwise this function
   * will trigger an assertion failure.
   *
   * @return std::vector<uint64_t> a list of all type IDs of all frozen types.
   */
  std::vector<uint64_t> GetTypeIds() const;

  /**
   * @brief Get the number of types.
   *
   * This function should only be called after this context has been frozen. Otherwise this function
   * will trigger an assertion failure.
   *
   * @return size_t the number of types.
   */
  size_t GetTypesCount() const;

  /**
   * @brief Get the constructors of the given type.
   *
   * This function should only be called after this context has been frozen. Otherwise this function
   * will trigger an assertion failure.
   *
   * @param type the type.
   * @return std::vector<const llvm::Function *> constructors of the given type.
   */
  std::vector<const llvm::Function *> GetConstructorsOfType(const llvm::Type* type) const;

  /**
   * @brief Get the constructing type of the given constructor.
   *
   * This function should only be called after this context has been frozen. Otherwise this function
   * will trigger an assertion failure.
   *
   * @param ctor the constructor.
   * @return Optional<const llvm::Type *> the constructing type of the given constructor. If the
   * given function is not a registered constructor, returns an empty Optional object.
   */
  Optional<const llvm::Type *> GetConstructingType(const llvm::Function* ctor) const;

  /**
   * @brief Get all constructors.
   *
   * This function should only be called after this context has been frozen. Otherwise this function
   * will trigger an assertion failure.
   *
   * @return std::vector<const llvm::Function *> all constructors.
   */
  std::vector<const llvm::Function *> GetConstructors() const;

  /**
   * @brief Get the number of constructors.
   *
   * This function should only be called after this context has been frozen. Otherwise this function
   * will trigger an assertion failure.
   *
   * @return size_t the number of constructors.
   */
  size_t GetConstructorsCount() const;

  /**
   * @brief Get the callback function candidates.
   *
   * This function should only be called after this context has been frozen. Otherwise this function
   * will trigger an assertion failure.
   *
   * @return const std::vector<Either<const llvm::Function *, LLVMFunctionSignature>> & list of
   * callback function candidates. If some element is const llvm::Function *, then the candidate is
   * present in the current LLVM module; otherwise it is not present in the current LLVM module.
   */
  auto GetCallbackFunctionCandidates() const ->
      const std::vector<Either<const llvm::Function *, LLVMFunctionSignature>> &;

  /**
   * @brief Get the number of callback functions.
   *
   * This function should only be called after this context has been frozen. Otherwise this function
   * will trigger an assertion failure.
   *
   * @return size_t the number of callback functions.
   */
  size_t GetCallbackFunctionsCount() const;

private:
  class FrozenContext;

  std::vector<const llvm::Function *> _apis;
  std::unordered_multimap<const llvm::Type *, const llvm::Function *> _ctors;
  std::vector<const llvm::Function *> _callbackFunctions;
  std::unique_ptr<FrozenContext> _frozen;

  void EnsureFrozen() const;
  void EnsureNotFrozen() const;

  void FreezeApiFunction(const llvm::Function* func) const;
  void FreezeType(const llvm::Type* type) const;
  void FreezeConstructor(const llvm::Type* type, const llvm::Function* func) const;
}; // class ExtractorContext

} // namespace caf

#endif
