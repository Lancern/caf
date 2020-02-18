#include "ConstructorVisitor.h"

namespace caf {

bool ConstructorVisitor::VisitCXXConstructorDecl(clang::CXXConstructorDecl* decl) {
  if (!decl->isInlined()) {
    // We're not interested in non-inline constructors.
    return true;
  }

  return _codeGen.EmitWrapperFunction(decl);
}

} // namespace caf
