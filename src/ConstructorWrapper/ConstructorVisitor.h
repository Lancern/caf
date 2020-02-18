#ifndef CAF_CONSTRUCTOR_VISITOR_H
#define CAF_CONSTRUCTOR_VISITOR_H

#include "CodeGenerator.h"

#include "clang/AST/RecursiveASTVisitor.h"

namespace clang {
class CompilerInstance;
} // namespace clang

namespace caf {

/**
 * @brief Implement clang::RecursiveASTVisitor to find all the inline constructors in the given
 * translation unit and wrap them with standalone exported wrapper functions.
 *
 */
class ConstructorVisitor : public clang::RecursiveASTVisitor<ConstructorVisitor> {
public:
  /**
   * @brief Construct a new ConstructorVisitor object.
   *
   * @param compiler the compiler instance.
   */
  explicit ConstructorVisitor(clang::CompilerInstance& compiler)
    : _codeGen { compiler }
  { }

  /**
   * @brief Process the given constructor declaration.
   *
   * @param decl the constructor declaration to process.
   * @return true if the visitor successfully processes the declaration.
   * @return false if the visitor cannot process the declaration.
   */
  bool VisitCXXConstructorDecl(clang::CXXConstructorDecl* decl);

private:
  CodeGenerator _codeGen;
}; // class ConstructorVisitor

} // namespace caf

#endif
