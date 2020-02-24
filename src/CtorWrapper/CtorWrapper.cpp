#include "CtorWrapperASTConsumer.h"
#include "CtorWrapperOpts.h"

#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/FrontendPluginRegistry.h"

#include <cstdlib>
#include <cstring>

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
    return llvm::make_unique<CtorWrapperASTConsumer>(compiler, _opts);
  }

  bool ParseArgs(const clang::CompilerInstance &, const std::vector<std::string>& args) override {
    for (const auto& arg : args) {
      if (arg == "-caf-ast-dump") {
        llvm::errs() << "CAF constructor wrapper plugin will dump AST after finishes.\n";
        _opts.DumpAST = true;
      }
    }

    return true;
  }

  clang::PluginASTAction::ActionType getActionType() override {
    return clang::PluginASTAction::AddBeforeMainAction;
  }

private:
  CtorWrapperOpts _opts;
}; // class CtorWrappper

namespace {

clang::FrontendPluginRegistry::Add<CtorWrapper> RegisterAction {
    "caf-ctor-wrappper", "CAF Constructor Wrapper Action" };

} // namespace <anonymous>

} // namespace caf
