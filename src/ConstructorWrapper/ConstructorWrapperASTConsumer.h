#ifndef CAF_CONSTRUCTOR_WRAPPER_AST_CONSUMER_H
#define CAF_CONSTRUCTOR_WRAPPER_AST_CONSUMER_H

#include "ConstructorVisitor.h"

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/CompilerInstance.h"

namespace caf {

/**
 * @brief Implement an ASTConsumer instance that is used to wrap each constructor inside the
 * translation unit with a standalone exported function. CAF leverages these functions to activate
 * instances of classes.
 *
 */
class ConstructorWrapperASTConsumer : public clang::ASTConsumer {
public:
  /**
   * @brief Construct a new ConstructorWrapperASTConsumer object.
   *
   * @param compiler the compiler instance.
   */
  explicit ConstructorWrapperASTConsumer(clang::CompilerInstance& compiler)
    : _visitor { compiler }
  { }

  /**
   * @brief Handle the translation unit.
   *
   * @param context clang::ASTContext instance containing the translation unit to process.
   */
  virtual void HandleTranslationUnit(clang::ASTContext& context) override;

private:
  ConstructorVisitor _visitor;
}; // class ConstructorWrapperASTConsumer

} // namespace caf

#endif
