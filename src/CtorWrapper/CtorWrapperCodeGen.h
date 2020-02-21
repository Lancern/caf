#ifndef CAF_CTOR_WRAPPER_CODE_GEN_H
#define CAF_CTOR_WRAPPER_CODE_GEN_H

#include <unordered_set>

namespace clang {
class ASTContext;
class CompilerInstance;
class CXXRecordDecl;
class CXXMethodDecl;
class CXXConstructorDecl;
class DeclarationName;
class DeclarationNameInfo;
class DeclContext;
class Expr;
class IdentifierInfo;
class QualType;
} // namespace clang

namespace caf {

/**
 * @brief Constructor wrapper function code generator.
 *
 */
class CtorWrapperCodeGen {
public:
  /**
   * @brief Construct a new CtorWrapperGen object.
   *
   * @param compiler the compiler instance.
   * @param context the AST context.
   */
  explicit CtorWrapperCodeGen(clang::CompilerInstance& compiler, clang::ASTContext& context)
    : _compiler(compiler),
      _context(context)
  { }

  /**
   * @brief Register the given constructor to the code generator.
   *
   * @param ctor the constructor.
   */
  void RegisterConstructor(clang::CXXConstructorDecl* ctor);

  /**
   * @brief Generate wrapper function code for all registered constructors.
   *
   */
  void GenerateCode();

private:
  clang::CompilerInstance& _compiler;
  clang::ASTContext& _context;
  std::unordered_set<clang::CXXConstructorDecl *> _ctors;

  clang::CXXMethodDecl* GenerateWrapperFuncMemberDeclFor(clang::CXXConstructorDecl* ctor);

  clang::CXXMethodDecl* GenerateWrapperFuncDefinitionFor(
      clang::CXXConstructorDecl* ctor, clang::CXXMethodDecl* decl);

  void GenerateWrapperFunctionBody(clang::CXXMethodDecl* func, clang::CXXConstructorDecl* ctor);

  clang::Expr* GenerateConstructArgument(
      clang::CXXMethodDecl* func, clang::CXXConstructorDecl* ctor, size_t argIndex);

  clang::CXXMethodDecl* CreateWrapperFuncDecl(
      clang::CXXConstructorDecl* ctor, clang::DeclContext* lexicalContext = nullptr);

  clang::QualType GetWrapperFunctionType(clang::CXXConstructorDecl* ctor);

  clang::IdentifierInfo* GetIdentifierInfo(const char* name);

  clang::DeclarationName GetDeclarationName(const char* name);

  clang::DeclarationNameInfo GetDeclarationNameInfo(const char* name);

  clang::CXXConstructorDecl* LookupMoveConstructor(clang::CXXRecordDecl* record);

  void EmitLLVM(clang::CXXMethodDecl* decl);
}; // class CtorWrapperCodeGen

} // namespace caf

#endif
