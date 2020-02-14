#include "Extractor/LLVMFunctionSignature.h"

#include "llvm/Support/Casting.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Function.h"

namespace caf {

LLVMFunctionSignature LLVMFunctionSignature::FromType(const llvm::Type* type) {
  return FromType(llvm::cast<llvm::FunctionType>(type));
}

LLVMFunctionSignature LLVMFunctionSignature::FromType(const llvm::FunctionType* type) {
  auto retType = type->getReturnType();
  auto paramTypes = type->params();
  return LLVMFunctionSignature { retType, paramTypes };
}

LLVMFunctionSignature LLVMFunctionSignature::FromFunction(const llvm::Function* func) {
  return FromType(func->getFunctionType());
}

bool operator==(const LLVMFunctionSignature& lhs, const LLVMFunctionSignature& rhs) {
  if (lhs.retType() != rhs.retType()) {
    return false;
  }
  if (lhs.paramTypes().size() != rhs.paramTypes().size()) {
    return false;
  }

  auto lhsp = lhs.paramTypes().begin();
  auto rhsp = rhs.paramTypes().begin();
  while (lhsp != lhs.paramTypes().end()) {
    if (*lhsp++ != *rhsp++) {
      return false;
    }
  }

  return true;
}

bool operator!=(const LLVMFunctionSignature& lhs, const LLVMFunctionSignature& rhs) {
  return !(operator==(lhs, rhs));
}

} // namespace caf
