#include "CtorWrapperASTConsumer.h"
#include "CtorWrapperCodeGen.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"

namespace caf {

namespace {

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
  return true;
}

class CtorWrapperASTVisitor : public clang::RecursiveASTVisitor<CtorWrapperASTVisitor> {
public:
  explicit CtorWrapperASTVisitor(clang::CompilerInstance& compiler, CtorWrapperCodeGen& cg)
    : _cg(cg)
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
  CtorWrapperCodeGen& _cg;
}; // class CtorWrapperASTVisitor

} // namespace <anonymous>

void CtorWrapperASTConsumer::HandleTranslationUnit(clang::ASTContext& context) {
  CtorWrapperCodeGen cg { _compiler, context };
  CtorWrapperASTVisitor visitor { _compiler, cg };
  visitor.TraverseTranslationUnitDecl(context.getTranslationUnitDecl());

  cg.GenerateCode();
}

} // namespace caf
