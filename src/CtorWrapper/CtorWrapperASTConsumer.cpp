#include "CtorWrapperASTConsumer.h"
#include "CtorWrapperCodeGen.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/Sema.h"

namespace caf {

namespace {

class CtorWrapperASTVisitor : public clang::RecursiveASTVisitor<CtorWrapperASTVisitor> {
public:
  explicit CtorWrapperASTVisitor(clang::CompilerInstance& compiler, CtorWrapperCodeGen& cg)
    : _compiler(compiler),
      _cg(cg)
  { }

  bool VisitCXXRecordDecl(clang::CXXRecordDecl* decl) {
    if (!IsInterestingRecordDecl(decl)) {
      return true;
    }

    for (auto ctorDecl : decl->ctors()) {
      if (!IsInterestingCtorDecl(ctorDecl)) {
        continue;
      }
      _cg.RegisterConstructor(ctorDecl);
    }

    return true;
  }

private:
  clang::CompilerInstance& _compiler;
  CtorWrapperCodeGen& _cg;

  bool IsInterestingRecordDecl(clang::CXXRecordDecl* decl) {
    if (!decl->hasDefinition()) {
      return false;
    }

    if (decl->isLambda()) {
      return false;
    }

    if (decl->isLocalClass()) {
      return false;
    }

    return true;
  }

  bool IsInterestingCtorDecl(clang::CXXConstructorDecl* decl) {
    auto& sema = _compiler.getSema();
    auto sm = sema.getSpecialMember(decl);
    if (sm == clang::Sema::CXXCopyConstructor || sm == clang::Sema::CXXMoveConstructor) {
      return false;
    }

    return true;
  }
}; // class CtorWrapperASTVisitor

} // namespace <anonymous>

void CtorWrapperASTConsumer::HandleTranslationUnit(clang::ASTContext& context) {
  CtorWrapperCodeGen cg { _compiler, context };
  CtorWrapperASTVisitor visitor { _compiler, cg };
  visitor.TraverseTranslationUnitDecl(context.getTranslationUnitDecl());

  cg.GenerateCode();
}

} // namespace caf
