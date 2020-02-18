#include "ConstructorWrapperASTConsumer.h"

namespace caf {

void ConstructorWrapperASTConsumer::HandleTranslationUnit(clang::ASTContext& context) {
  llvm::errs() << "CAF plugin is handling translation unit\n";
  _visitor.TraverseDecl(context.getTranslationUnitDecl());
}

} // namespace caf
