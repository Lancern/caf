#include "Basic/CAFStore.h"
#include "Basic/JsonSerializer.h"
#include "Extractor/ExtractorPass.h"
#include "ExtractorUtils.h"

#define DEBUG_TYPE "CAFExtractor"

#include "llvm/Pass.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

#include "json/json.hpp"

#include <cstring>
#include <utility>
#include <string>

namespace caf {

namespace {

llvm::cl::opt<std::string> CAFStoreOutputFileName(
    "cafstore",
    llvm::cl::desc("Specify the path to which the CAF store data will be serialized."),
    llvm::cl::value_desc("path"));

llvm::RegisterPass<ExtractorPass> RegisterExtractor {
    "cafextractor", "CAF Metadata Extractor Pass", false, true };

STATISTIC(ApiFunctionsCount, "Number of API functions available as fuzz target");

STATISTIC(ConstructorsCount, "Number of constructors found in the module");
STATISTIC(CallbackCandidatesCount, "Number of callback function candidates in the module");

STATISTIC(FrozenTypesCount, "Number of frozen types");
STATISTIC(FrozenConstructorsCount, "Number of frozen constructors");
STATISTIC(FrozenCallbackCandidatesCount, "Number of frozen callback function candidates");

} // namespace <anonymous>

ExtractorPass::ExtractorPass()
  : llvm::ModulePass { ID }
{ }

char ExtractorPass::ID = 0;

bool ExtractorPass::runOnModule(llvm::Module &module) {
  for (auto& func : module) {
    if (caf::IsApiFunction(func)) {
      ++ApiFunctionsCount;
      _context.AddApiFunction(&func);
    } else if (caf::IsConstructor(func)) {
      ++ConstructorsCount;
      auto name = caf::GetConstructingTypeName(func);
      _context.AddConstructor(std::move(name), &func);
    }

    ++CallbackCandidatesCount;
    _context.AddCallbackFunctionCandidate(&func);
  }

  _context.Freeze();

  FrozenTypesCount = _context.GetTypesCount();
  FrozenConstructorsCount = _context.GetConstructorsCount();
  FrozenCallbackCandidatesCount = _context.GetCallbackFunctionsCount();

  auto outputFileName = CAFStoreOutputFileName.c_str();
  if (outputFileName && strlen(outputFileName)) {
    do {
      std::error_code openFileError { };
      llvm::raw_fd_ostream outputFile {outputFileName, openFileError };
      if (openFileError) {
        llvm::errs() << "CAFDriver: failed to dump metadata to file: "
            << openFileError.value() << ": "
            << openFileError.message() << "\n";
        break;
      }

      auto store = _context.CreateCAFStore();
      caf::JsonSerializer jsonSerializer;
      auto json = jsonSerializer.Serialize(*store);
      outputFile << json.dump();
      outputFile.flush();
      outputFile.close();

      llvm::errs() << "CAF store data has been saved to " << outputFileName << "\n";
    } while (false);
  } else {
    llvm::errs() << "CAF store data will not be persisted since -cafstore is not provided.\n";
  }

  return false;
}

void ExtractorPass::getAnalysisUsage(llvm::AnalysisUsage& usage) const {
  usage.setPreservesAll();
}

} // namespace caf
