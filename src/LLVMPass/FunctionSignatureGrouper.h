#ifndef CAF_FUNCTION_SIGNATURE_GROUPER_H
#define CAF_FUNCTION_SIGNATURE_GROUPER_H

#include "LLVMFunctionSignature.h"
#include "Infrastructure/Hash.h"

#include <unordered_map>
#include <vector>

namespace llvm {
class Function;
}; // namespace llvm

namespace caf {

/**
 * @brief Groups LLVM functions by their function signatures.
 *
 */
class FunctionSignatureGrouper {
public:
  /**
   * @brief Construct a new @see FunctionSignatureGrouper object.
   *
   */
  explicit FunctionSignatureGrouper() = default;

  FunctionSignatureGrouper(const FunctionSignatureGrouper &) = delete;
  FunctionSignatureGrouper(FunctionSignatureGrouper &&) noexcept = default;

  FunctionSignatureGrouper& operator=(const FunctionSignatureGrouper &) = delete;
  FunctionSignatureGrouper& operator=(FunctionSignatureGrouper &&) = default;

  /**
   * @brief Clear all functions and function signatures registered.
   *
   */
  void clear();

  /**
   * @brief Register the given LLVM function to this @see FunctionSignatureGrouper.
   *
   * If the signature of the given function already exists, then the given function will be added
   * to the same group as those previously added functions with the same signature; otherwise a new
   * signature group will be created and the given function will be added to it.
   *
   * @param function the LLVM function to register.
   */
  void Register(llvm::Function* function);

  /**
   * @brief Get all functions that matches the given signature.
   *
   * @param signature the function signature.
   * @return const std::vector<llvm::Function *>* pointer to a list of functions that matches the
   * given signature. If no functions match the given signature, returns nullptr.
   */
  const std::vector<llvm::Function *>* GetFunctions(const LLVMFunctionSignature& signature) const;

private:
  std::unordered_map<
      LLVMFunctionSignature,
      std::vector<llvm::Function *>,
      Hasher<LLVMFunctionSignature>> _signatures;

  std::vector<llvm::Function *>& GetOrAddSignature(LLVMFunctionSignature&& signature);
}; // class FunctionSignatureGrouper

} // namespace caf

#endif
