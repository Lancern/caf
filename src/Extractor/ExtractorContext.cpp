#include "Infrastructure/Memory.h"
#include "Basic/CAFStore.h"
#include "Basic/Function.h"
#include "Extractor/ExtractorContext.h"

#include "llvm/IR/Function.h"

#include <cassert>

namespace caf {

void ExtractorContext::AddFunction(llvm::Function* function) {
  assert(function && "Trying to add a nullptr.");
  _funcs.push_back(function);
}

std::unique_ptr<CAFStore> ExtractorContext::CreateStore() const {
  std::vector<Function> funcs;
  funcs.reserve(_funcs.size());

  FunctionIdType funcId = 0;
  for (auto llvmFunc : _funcs) {
    Function cafFunc { funcId++, std::string(llvmFunc->getName()) };
    funcs.push_back(std::move(cafFunc));
  }

  return caf::make_unique<CAFStore>(std::move(funcs));
}

} // namespace caf
