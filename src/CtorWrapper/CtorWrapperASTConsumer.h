#ifndef CAF_CTOR_WRAPPER_AST_CONSUMER_H
#define CAF_CTOR_WRAPPER_AST_CONSUMER_H

#include "CtorWrapperOpts.h"

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
  explicit CtorWrapperASTConsumer(clang::CompilerInstance& compiler, CtorWrapperOpts opts)
    : _compiler(compiler),
      _opts(opts)
  { }

  void HandleTranslationUnit(clang::ASTContext& context) override;

private:
  clang::CompilerInstance& _compiler;
  CtorWrapperOpts _opts;
}; // class CtorWrapperASTConsumer

} // namespace caf

#endif
