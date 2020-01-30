#ifndef CAF_LLVM_FUNCTION_SIGNATURE_H
#define CAF_LLVM_FUNCTION_SIGNATURE_H

#include "Infrastructure/Hash.h"

#include "llvm/ADT/ArrayRef.h"

namespace llvm {
class Type;
class FunctionType;
class Function;
}; // namespace llvm

namespace caf {

/**
 * @brief Function signature represented in LLVM's type hierarchy.
 *
 */
class LLVMFunctionSignature {
public:
  /**
   * @brief Construct a new LLVMFunctionSignature object.
   *
   * @param retType the function's return type.
   * @param paramTypes the types of the parameters.
   */
  explicit LLVMFunctionSignature(llvm::Type* retType, llvm::ArrayRef<llvm::Type *> paramTypes)
    : _retType(retType),
      _paramTypes(paramTypes)
  { }

  /**
   * @brief Get the function's return type.
   *
   * @return llvm::Type* the function's return type.
   */
  llvm::Type* retType() const { return _retType; }

  /**
   * @brief Get the function's parameter types.
   *
   * @return llvm::ArrayRef<llvm::Type *> the function's parameter types.
   */
  llvm::ArrayRef<llvm::Type *> paramTypes() const { return _paramTypes; }

  /**
   * @brief Create a new @see LLVMFunctionSignature object from the given LLVM type.
   *
   * If the given LLVM type is not a function type, then an assertion failure will be raised.
   *
   * @param type the LLVM type.
   * @return LLVMFunctionSignature the function signature of the given type.
   */
  static LLVMFunctionSignature FromType(llvm::Type* type);

  /**
   * @brief Create a new @see LLVMFunctionSignature object from the given LLVM function type.
   *
   * @param type the LLVM function type.
   * @return LLVMFunctionSignature the function signature of the given type.
   */
  static LLVMFunctionSignature FromType(llvm::FunctionType* type);

  /**
   * @brief Create a new @see LLVMFunctionSignature object representing the given LLVM function's
   * function signature.
   *
   * @param func the LLVM function.
   * @return LLVMFunctionSignature the function signature of the given LLVM function.
   */
  static LLVMFunctionSignature FromFunction(llvm::Function* func);

private:
  llvm::Type* _retType;
  llvm::ArrayRef<llvm::Type *> _paramTypes;
};

bool operator==(const LLVMFunctionSignature& lhs, const LLVMFunctionSignature& rhs);
bool operator!=(const LLVMFunctionSignature& lhs, const LLVMFunctionSignature& rhs);

template <>
struct Hasher<LLVMFunctionSignature> {
  size_t operator()(const LLVMFunctionSignature& signature) const {
    return CombineHash(
        GetHashCode(signature.retType()),
        GetContainerHashCode(signature.paramTypes()));
  }
}; // struct Hasher<LLVMFunctionSignature>

}; // namespace caf

#endif
