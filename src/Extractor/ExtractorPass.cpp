#include "Infrastructure/Casting.h"
#include "Basic/CAFStore.h"
#include "Basic/JsonSerializer.h"
#include "Extractor/ExtractorPass.h"
#include "ExtractorUtils.h"

#define DEBUG_TYPE "CAFExtractor"

#include "llvm/Pass.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
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

llvm::cl::opt<bool> DetectV8ApiFunctions(
    "detect-v8-apis",
    llvm::cl::desc("Specify whether to detect V8 JavaScript engine API functions"),
    llvm::cl::init(false));

llvm::RegisterPass<ExtractorPass> RegisterExtractor {
    "cafextractor", "CAF Metadata Extractor Pass", false, true };

STATISTIC(ApiFunctionsCount, "Number of API functions found in the module");
STATISTIC(V8ApiFunctionCount, "Number of V8 JavaScript engine API functions found in the module");

STATISTIC(ConstructorsCount, "Number of constructors found in the module");
STATISTIC(CallbackCandidatesCount, "Number of callback function candidates in the module");

STATISTIC(FrozenApiFunctionsCount, "Number of frozen API functions");
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
    } else if (DetectV8ApiFunctions && caf::IsV8ApiFunction(func)) {
      ++V8ApiFunctionCount;
      for (const auto& bb : func) {
        for (const auto& instr : bb) {
          llvm::Function* callee = nullptr;
          if (caf::is_a<llvm::CallInst>(instr)) {
            callee = caf::dyn_cast<llvm::CallInst>(instr).getCalledFunction();
          } else if (caf::is_a<llvm::InvokeInst>(instr)) {
            callee = caf::dyn_cast<llvm::InvokeInst>(instr).getCalledFunction();
          }
          if (callee && !callee->isIntrinsic()) {
            ++ApiFunctionsCount;
            _context.AddApiFunction(callee);
          }
        }
      }
    } else if (caf::IsConstructor(func)) {
      ++ConstructorsCount;
      // auto name = caf::GetConstructingTypeName(func);
      _context.AddConstructor(caf::GetConstructingType(&func), &func);
    }

    ++CallbackCandidatesCount;
    _context.AddCallbackFunctionCandidate(&func);
  }

  _context.Freeze();

  FrozenApiFunctionsCount = _context.GetApiFunctionsCount();
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
