#include "Utils.h"
#include "Basic/CAFStore.h"
#include "Extractor/ExtractorPass.h"

#define DEBUG_TYPE "cafextractor"
#include "llvm/Pass.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/IR/Module.h"

#include <cerrno>
#include <cstring>
#include <string>
#include <fstream>

namespace caf {

namespace {

llvm::RegisterPass<ExtractorPass> X { "cafextractor", "CAF Extractor Pass", false, true };

STATISTIC(ApiCount, "Number of API functions");

llvm::cl::opt<std::string> CAFStoreFileName {
    "cafstore", llvm::cl::desc("Path to the cafstore.json file"), llvm::cl::value_desc("PATH") };

} // namespace <anonymous>

char ExtractorPass::ID = 0;

ExtractorPass::ExtractorPass()
  : llvm::ModulePass { ID }
{ }

void ExtractorPass::getAnalysisUsage(llvm::AnalysisUsage& usage) const {
  usage.getPreservesAll();
}

bool ExtractorPass::runOnModule(llvm::Module& module) {
  for (auto& func : module) {
    if (!IsApiFunction(&func)) {
      continue;
    }
    _context.AddFunction(&func);
    ++ApiCount;
  }

  auto store = _context.CreateStore();
  auto json = store->ToJson();

  auto storeFileName = CAFStoreFileName.getValue();
  if (storeFileName.empty()) {
    storeFileName = std::string("cafstore.json");
  }

  std::ofstream storeFileStream { storeFileName };
  if (storeFileStream.fail()) {
    auto errorCode = errno;
    auto errorMsg = std::strerror(errorCode);
    llvm::errs() << "Open CAF Store file failed: " << errorMsg << " (" << errorCode << ")\n";
    return false;
  }

  storeFileStream << json;
  storeFileStream.flush();
  storeFileStream.close();

  llvm::errs() << "CAF store has been saved to " << storeFileName << "\n";
  return false;
}

} // namespace caf
