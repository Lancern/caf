#ifndef CAF_CODE_GENERATOR_H
#define CAF_CODE_GENERATOR_H

#include "clang/AST/DeclarationName.h"
#include "clang/AST/Type.h"
#include "clang/Frontend/CompilerInstance.h"

namespace clang {
class CXXConstructorDecl;
} // namespace clang

namespace caf {

/**
 * @brief Provide a generator that generates wrapper functions of constructors.
 *
 */
class CodeGenerator {
public:
  /**
   * @brief Construct a new CodeGenerator object.
   *
   * @param compiler the compiler instance.
   */
  explicit CodeGenerator(clang::CompilerInstance& compiler)
    : _compiler(compiler)
  { }

  /**
   * @brief Emit a wrapper function for the given constructor.
   *
   * @param ctor the constructor.
   * @return true if the wrapper function has been succesfully emitted.
   * @return false if something goes wrong.
   */
  bool EmitWrapperFunction(clang::CXXConstructorDecl* ctor);

private:
  clang::CompilerInstance& _compiler;

  /**
   * @brief Emit the given top level declaration by invoking AST consumers with this declaration.
   *
   * @param decl the declaration to invoke.
   * @return true if the AST consumer successfully handled the declaration.
   * @return false otherwise.
   */
  bool emitTopLevelDecl(clang::Decl* decl);

  /**
   * @brief Create a wrapper function wrapping around the given constructor.
   *
   * The argument list of the created wrapper function will be identical to the constructor's
   * argument list. The wrapper function simply creates a new object and calls the given constructor
   * with the argument list given. A pointer to the initialized object will then be returned from
   * the wrapper function.
   *
   * To ensure that wrapper functions around the same constructor can safely reside in multiple
   * translation units, the returned wrapper function will have the `weak` attribute to mark the
   * function as a weak symbol when emitting LLVM modules.
   *
   * @param ctor the constructor to be wrapped.
   * @return clang::FunctionDecl* pointer to the created wrapping function around the given
   * constructor.
   */
  clang::FunctionDecl* createConstructorWrapperFunction(clang::CXXConstructorDecl* ctor);

  /**
   * @brief Get a clang::DeclarationName value indicating the name of the wrapper function around
   * the given constructor.
   *
   * @param ctor the constructor being wrapped by the wrapper function.
   * @return clang::DeclarationName the name of the wrapper function.
   */
  clang::DeclarationName getWrapperFunctionName(clang::CXXConstructorDecl* ctor);

  /**
   * @brief Create a clang::QualType value representing the type of the wrapper function around the
   * given constructor.
   *
   * @param decl the constructor being wrapped by the wrapper function.
   * @return clang::QualType the qualified type of the wrapper function.
   */
  clang::QualType getWrapperFunctionType(const clang::CXXConstructorDecl* ctor);

  /**
   * @brief Populate the body of the given wrapper function that wraps around the given
   * constructor.
   *
   * @param wrapperFunc The wrapper function onto which the body statements will be populated.
   * @param ctor The constructor wrapped by the given wrapper function.
   * @return clang::FunctionDecl* Pointer to the given wrapper function.
   */
  clang::FunctionDecl* populateWrapperFunctionBody(
      clang::FunctionDecl* wrapperFunc, clang::CXXConstructorDecl* ctor);
};

} // namespace caf

#endif
