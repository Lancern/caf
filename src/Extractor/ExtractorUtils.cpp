#include "Infrastructure/Casting.h"
#include "ExtractorUtils.h"

#include "llvm/IR/Attributes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/CommandLine.h"

#include <cxxabi.h>

namespace caf {

bool IsApiFunction(const llvm::Function& func) {
  if (func.isDeclaration()) {
    return false;
  }

  return func.hasFnAttribute(llvm::Attribute::CafApi);
}

bool IsV8ApiFunction(const llvm::Function& func) {
  if (func.isDeclaration()) {
    return false;
  }

  auto funcType = func.getFunctionType();
  if (funcType->getNumParams() != 1) {
    return false;
  }

  auto paramType = funcType->getParamType(0);
  if (!paramType->isPointerTy()) {
    return false;
  }

  auto pointeeType = paramType->getPointerElementType();
  if (!pointeeType->isStructTy()) {
    return false;
  }

  auto structType = caf::dyn_cast<llvm::StructType>(pointeeType);
  if (!structType->hasName()) {
    return false;
  }

  return pointeeType->getStructName() == "class.v8::FunctionCallbackInfo";
}

bool IsConstructor(const llvm::Function& func) {
  if (!func.hasFnAttribute(llvm::Attribute::CafCxxCtor)) {
    return false;
  }

  auto funcType = func.getFunctionType();
  if (funcType->getNumParams() == 0) {
    return false;
  }

  if (!funcType->getParamType(0)->isPointerTy()) {
    return false;
  }

  return true;
}

llvm::Type* GetConstructingType(llvm::Function* ctor) {
  auto ctorFuncType = ctor->getFunctionType();
  if (ctorFuncType->getNumParams() == 0) {
    llvm::errs() << ctor->getName() << "\n";
  }

  assert(ctorFuncType->getNumParams() >= 1 && "Constructors should have at least 1 parameter.");

  auto firstParam = ctorFuncType->getParamType(0);
  assert(firstParam->isPointerTy() && "The first parameter of a constructor should be a pointer.");

  return firstParam->getPointerElementType();
}

} // namespace caf
