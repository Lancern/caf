#include "Extractor/ExtractorPass.h"
// #include "common/CAFCodeGenerator.h"
#include "Instrumentor/InstrumentorPass.h"
#include "Instrumentor/nodejs/CAFCodeGenerator.h"
#include "Extractor/ExtractorPass.h"

#include "llvm/Pass.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/IR/Module.h"

namespace caf {

namespace {

  static llvm::RegisterPass<InstrumentorPass> RegInstrumentor {
    "cafinstrumentor", "CAF Instrumentor Pass", false, true };
  
  llvm::cl::opt<std::string> CAFInstrumentorTarget {
    "", 
    llvm::cl::desc("Caf target of instrmentation"), 
    llvm::cl::init("common")
  };

} // namespace <anonymous>

char InstrumentorPass::ID = 0;

InstrumentorPass::InstrumentorPass()
  : llvm::ModulePass { ID }
{ }

void InstrumentorPass::getAnalysisUsage(llvm::AnalysisUsage& usage) const {
  usage.getPreservesAll();
}

bool InstrumentorPass::runOnModule(llvm::Module& module) {
  const auto& extractions = getAnalysis<ExtractorPass>().GetContext();

   auto cafInstrumentorTarget = CAFInstrumentorTarget.getValue();
  if(cafInstrumentorTarget == "nodejs") {
    CAFCodeGenerator generator { };
    generator.SetContext(module, extractions);
    generator.GenerateStub();
  } else { // common target
    llvm::errs() << "common target.\n";
  }

  return true;
}

} // namespace caf
