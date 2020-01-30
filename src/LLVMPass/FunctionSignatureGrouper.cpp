#include "FunctionSignatureGrouper.h"

#include "llvm/IR/Function.h"

namespace caf {

void FunctionSignatureGrouper::clear() {
  _signatures.clear();
}

void FunctionSignatureGrouper::Register(llvm::Function* function) {
  auto signature = LLVMFunctionSignature::FromFunction(function);
  GetOrAddSignature(std::move(signature)).push_back(function);
}

const std::vector<llvm::Function *>* FunctionSignatureGrouper::GetFunctions(
    const LLVMFunctionSignature& signature) {
  auto i = _signatures.find(signature);
  if (i == _signatures.end()) {
    return nullptr;
  }

  return &i->second;
}

std::vector<llvm::Function *>& FunctionSignatureGrouper::GetOrAddSignature(
    LLVMFunctionSignature&& signature) {
  auto i = _signatures.find(signature);
  if (i == _signatures.end()) {
    return _signatures.emplace(std::move(signature), std::vector<llvm::Function *> { })
        .first->second;
  }
  return i->second;
}

} // namespace caf
