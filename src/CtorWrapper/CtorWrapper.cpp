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
    CtorWrapperOpts opts;
    opts.DumpAST = true;

    // auto dumpASTEnv = std::getenv("CAF_DUMP_AST");
    // if (dumpASTEnv && std::strcmp(dumpASTEnv, "TRUE")) {
    //   llvm::errs() << "CAF constructor wrapper will dump AST after finish.\n";
    //   opts.DumpAST = true;
    // }

    return llvm::make_unique<CtorWrapperASTConsumer>(compiler, opts);
  }

  bool ParseArgs(const clang::CompilerInstance &, const std::vector<std::string> &) override {
    return true;
  }

  clang::PluginASTAction::ActionType getActionType() override {
    return clang::PluginASTAction::AddBeforeMainAction;
  }
}; // class CtorWrappper

namespace {

clang::FrontendPluginRegistry::Add<CtorWrapper> RegisterAction {
    "caf-ctor-wrappper", "CAF Constructor Wrapper Action" };

} // namespace <anonymous>

} // namespace caf
