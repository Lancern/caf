#include "Utils.h"

#include "llvm/Support/Casting.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"

#include <cassert>

namespace caf {

namespace {

bool IsV8ApiFunction(const llvm::Function* func) {
  auto funcType = func->getFunctionType();
  if (funcType->getNumParams() != 1) {
    return false;
  }

  auto firstParam = funcType->getParamType(0);
  if (!firstParam->isPointerTy()) {
    return false;
  }
  firstParam = firstParam->getPointerElementType();

  if (!firstParam->isStructTy()) {
    return false;
  }

  auto firstParamStruct = llvm::cast<llvm::StructType>(firstParam);
  if (firstParamStruct->isLiteral() || !firstParamStruct->hasName()) {
    return false;
  }

  auto firstParamStructName = firstParamStruct->getStructName();
  return firstParamStructName.startswith("class.v8::FunctionCallbackInfo");
}

} // namespace <anonymous>

bool IsApiFunction(const llvm::Function* func) {
  assert(func && "func is null.");

  if (func->isIntrinsic()) {
    return false;
  }

  if (func->isDeclaration()) {
    return false;
  }

  return IsV8ApiFunction(func);
}

} // namespace caf
