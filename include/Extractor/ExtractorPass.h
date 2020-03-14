#ifndef CAF_EXTRACTOR_PASS_H
#define CAF_EXTRACTOR_PASS_H

#include "Extractor/ExtractorContext.h"

#include "llvm/Pass.h"

namespace llvm {
class AnalysisUsage;
class Module;
}

namespace caf {

/**
 * @brief CAF extractor pass.
 *
 */
class ExtractorPass : public llvm::ModulePass {
public:
  /**
   * @brief Construct a new ExtractorPass object.
   *
   */
  explicit ExtractorPass();

  void getAnalysisUsage(llvm::AnalysisUsage& usage) const override;

  bool runOnModule(llvm::Module& module) override;

  /**
   * @brief Get the extractor context.
   *
   * @return const ExtractorContext& the extractor context.
   */
  const ExtractorContext& GetContext() const { return _context; }

  /**
   * @brief Reserved for LLVM use.
   *
   */
  static char ID;

private:
  ExtractorContext _context;
}; // class ExtractorPass

} // namespace caf

#endif
