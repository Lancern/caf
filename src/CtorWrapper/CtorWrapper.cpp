#include "CtorWrapperASTConsumer.h"

#include "clang/Frontend/FrontendAction.h"

namespace clang {
class CompilerInstance;
} // namespace clang

namespace caf {

/**
 * @brief Implement an AST action that wraps constructors into static methods of defining classes.
 *
 */
class CtorWrapper : public clang::PluginASTAction {
protected:
  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
      clang::CompilerInstance& compiler, llvm::StringRef) override {
    return llvm::make_unique<CtorWrapperASTConsumer>(compiler);
  }

  bool ParseArgs(const clang::CompilerInstance &, const std::vector<std::string> &) override {
    return true;
  }

  clang::PluginASTAction::ActionType getActionType() override {
    return clang::PluginASTAction::AddBeforeMainAction;
  }
}; // class CtorWrappper

} // namespace caf
