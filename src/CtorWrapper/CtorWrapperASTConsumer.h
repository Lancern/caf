#ifndef CAF_CTOR_WRAPPER_AST_CONSUMER_H
#define CAF_CTOR_WRAPPER_AST_CONSUMER_H

#include "clang/AST/ASTConsumer.h"

namespace clang {
class CompilerInstance;
} // namespace clang

namespace caf {

/**
 * @brief AST consumer for the CtorWrapper AST action.
 *
 */
class CtorWrapperASTConsumer : public clang::ASTConsumer {
public:
  /**
   * @brief Construct a new CtorWrapperASTConsumer object.
   *
   * @param compiler the compiler instance.
   */
  explicit CtorWrapperASTConsumer(clang::CompilerInstance& compiler)
    : _compiler(compiler)
  { }

  void HandleTranslationUnit(clang::ASTContext& context) override;

private:
  clang::CompilerInstance& _compiler;
}; // class CtorWrapperASTConsumer

} // namespace caf

#endif
