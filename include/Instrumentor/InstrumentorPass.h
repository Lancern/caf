#ifndef CAF_INSTRUMENTOR_PASS_H
#define CAF_INSTRUMENTOR_PASS_H

#include "llvm/Pass.h"

namespace llvm {
class AnalysisUsage;
class Module;
}

namespace caf {

class InstrumentorPass : public llvm::ModulePass {
public:
  explicit InstrumentorPass();

  bool runOnModule(llvm::Module& module) override;

  void getAnalysisUsage(llvm::AnalysisUsage& usage) const override;

  static char ID;
}; // class InstrumentorPass

} // namespace caf

#endif
