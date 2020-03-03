#include "Extractor/ExtractorPass.h"
#include "CAFCodeGenerator.h"

#include "llvm/Pass.h"

namespace caf {

class InstrumentorPass : public llvm::ModulePass {
public:
  explicit InstrumentorPass()
    : llvm::ModulePass { ID }
  { }

  bool runOnModule(llvm::Module& module) override {
    const auto& extractions = getAnalysis<ExtractorPass>().context();

    CAFCodeGenerator generator { };
    generator.SetContext(module, extractions);
    generator.GenerateCallbackFunctionCandidateArray(extractions.GetCallbackFunctionCandidates());
    generator.GenerateStub();

    return true;
  }

  void getAnalysisUsage(llvm::AnalysisUsage& usage) const override {
    usage.addRequired<ExtractorPass>();
    usage.addPreserved<ExtractorPass>();
  }

  static char ID;
}; // class InstrumentorPass

char InstrumentorPass::ID = 0;

static llvm::RegisterPass<InstrumentorPass> RegInstrumentor {
    "cafinstrumentor", "CAF Instrumentor Pass", false, false };

} // namespace caf
