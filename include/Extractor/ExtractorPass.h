#ifndef CAF_EXTRACTOR_PASS_H
#define CAF_EXTRACTOR_PASS_H

#include "Extractor/ExtractorContext.h"

#include "llvm/Pass.h"

namespace caf {

/**
 * @brief CAF extractor analysis pass.
 *
 */
class ExtractorPass : public llvm::ModulePass {
public:
  /**
   * @brief Construct a new ExtractorPass object.
   *
   */
  explicit ExtractorPass();

  /**
   * @brief Get the context of the extractor pass.
   *
   * @return ExtractorContext& context of the extractor pass.
   */
  ExtractorContext& context() { return _context; }

  /**
   * @brief Get the context of the extractor pass.
   *
   * @return const ExtractorContext& context of the extractor pass.
   */
  const ExtractorContext& context() const { return _context; }

  bool runOnModule(llvm::Module& module) override;
  void getAnalysisUsage(llvm::AnalysisUsage& usage) const override;

  static char ID;

private:
  ExtractorContext _context;
}; // class ExtractorPass

} // namespace caf

#endif
