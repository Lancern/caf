#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/Sema.h"

#include "llvm/Support/raw_ostream.h"

#include <string>

namespace {

/**
 * @brief Implement clang::RecursiveASTVisitor to find all the inline constructors in the given
 * translation unit and wrap them with standalone exported wrapper functions.
 *
 */
class CAFFindInlineConstructorVisitor
    : public clang::RecursiveASTVisitor<CAFFindInlineConstructorVisitor> {
public:
  /**
   * @brief Construct a new CAFFindInlineConstructorVisitor object.
   *
   * @param compiler the compiler instance.
   */
  explicit CAFFindInlineConstructorVisitor(clang::CompilerInstance& compiler)
    : _compiler(compiler)
  { }

  /**
   * @brief Process the given constructor declaration.
   *
   * @param decl the constructor declaration to process.
   * @return true if the visitor successfully processes the declaration.
   * @return false if the visitor cannot process the declaration.
   */
  bool VisitCXXConstructorDecl(clang::CXXConstructorDecl* decl) {
    if (!decl->isInlined()) {
      // We're not interested in non-inline constructors.
      return true;
    }

    createConstructorWrapperFunction(decl);
  } // bool VisitCXXConstructorDecl

private:
  clang::CompilerInstance& _compiler;

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
  clang::FunctionDecl* createConstructorWrapperFunction(clang::CXXConstructorDecl* ctor) {
    auto& astContext = ctor->getASTContext();
    auto wrapperDeclContext = findWrapperContext(ctor);
    auto wrapperName = getWrapperFunctionName(ctor);
    auto wrapperType = getWrapperFunctionType(ctor);
    auto wrapperFunc = clang::FunctionDecl::Create(
        astContext,
        wrapperDeclContext,
        clang::SourceLocation { },
        clang::SourceLocation { },
        wrapperName,
        wrapperType,
        nullptr,
        // Set storage class to SC_None allows clang to mark the function as exported when emitting
        // LLVM code.
        clang::StorageClass::SC_None);

    // Add weak symbol attribute to the created wrapper function.
    // auto weakAttr = new (astContext) clang::Attr(
    //     clang::attr::Kind::Weak,
    //     clang::SourceRange { },
    //     /* SpellingListIndex: */ 0,
    //     /* IsLateParsed: */ false);
    // wrapperFunc->addAttr(weakAttr);

    return populateWrapperFunctionBody(wrapperFunc, ctor);
  }

  /**
   * @brief Get a clang::DeclarationName value indicating the name of the wrapper function around
   * the given constructor.
   *
   * @param ctor the constructor being wrapped by the wrapper function.
   * @return clang::DeclarationName the name of the wrapper function.
   */
  clang::DeclarationName getWrapperFunctionName(const clang::CXXConstructorDecl* ctor) {
    auto& identifierTable = _compiler.getPreprocessor().getIdentifierTable();

    std::string name { };
    name.append("CAFCallCtor");
    name.append(ctor->getDeclName().getAsIdentifierInfo()->getNameStart());

    return clang::DeclarationName { &identifierTable.get(name) };
  }

  /**
   * @brief Create a clang::QualType value representing the type of the wrapper function around the
   * given constructor.
   *
   * @param decl the constructor being wrapped by the wrapper function.
   * @return clang::QualType the qualified type of the wrapper function.
   */
  clang::QualType getWrapperFunctionType(const clang::CXXConstructorDecl* ctor) {
    auto& astContext = ctor->getASTContext();

    std::vector<clang::QualType> args { };
    // The arguments' types are identical to the constructor's arguments' types.
    for (auto paramDecl : ctor->parameters()) {
      args.push_back(paramDecl->getType());
    }

    // The return type should be identical to the type of `this` pointer inside the constructor.
    auto returnType = ctor->getThisType(astContext);

    return astContext.getFunctionType(returnType, args, clang::FunctionProtoType::ExtProtoInfo { });
  }

  /**
   * @brief Find the declaration context into which the wrapper function for the given constructor
   * can be inserted.
   *
   * The returned declaration context will be the declaring context of the class declaraing the
   * given constructor.
   *
   * @param wrappedCtor the wrapped constructor of the wrapping function to be inserted.
   * @return clang::DeclContext* the declaration context into which the wrapping function can be
   * inserted.
   */
  clang::DeclContext* findWrapperContext(clang::CXXConstructorDecl* wrappedCtor) {
    auto context = static_cast<clang::DeclContext *>(wrappedCtor->getParent());
    while (!llvm::isa<clang::CXXRecordDecl>(context)) {
      context = context->getParent();
    }
    return context->getParent();
  }

  /**
   * @brief Populate the body of the given wrapper function that wraps around the given
   * constructor.
   *
   * @param wrapperFunc The wrapper function onto which the body statements will be populated.
   * @param ctor The constructor wrapped by the given wrapper function.
   * @return clang::FunctionDecl* Pointer to the given wrapper function.
   */
  clang::FunctionDecl* populateWrapperFunctionBody(
      clang::FunctionDecl* wrapperFunc, clang::CXXConstructorDecl* ctor) {
    auto& astContext = ctor->getASTContext();

    // The function body of the wrapper function is a simple return new T(...) statement where T is
    // the type declaring the constructor given.
    auto ctorParent = ctor->getParent();

    // Make an array of clang::DeclRefExpr * denoting the arguments list that will be passed to the
    // constructor. Each entry in this vector is a reference to the wrapper function's argument.
    std::vector<clang::Expr *> declRefs;
    declRefs.reserve(wrapperFunc->getNumParams());
    for (auto paramDecl : wrapperFunc->parameters()) {
      declRefs.push_back(clang::DeclRefExpr::Create(
          astContext,
          clang::NestedNameSpecifierLoc { },
          clang::SourceLocation { },
          paramDecl,
          false,
          clang::SourceLocation { },
          paramDecl->getType(),
          clang::ExprValueKind::VK_LValue));
    }

    auto constructingType = astContext.getRecordType(ctorParent);
    auto constructExpr = clang::CXXConstructExpr::Create(
        astContext,
        constructingType,
        clang::SourceLocation { },
        ctor,
        /* Elidable: */ true,
        declRefs,
        /* HadMultipleCandidates: */ false,
        /* ListInitialization: */ false,
        /* StdListInitialization: */ false,
        /* ZeroInitialization: */ false,
        clang::CXXConstructExpr::ConstructionKind::CK_Complete,
        clang::SourceRange { });
    auto operatorNew = lookupNewOperator(astContext, ctorParent);
    auto operatorDelete = lookupDeleteOperator(astContext, ctorParent);

    auto newExpr = new (astContext) clang::CXXNewExpr(
        astContext,
        /* GlobalNew: */ false,
        operatorNew,
        operatorDelete,
        /* PassAlignment: */ false,
        /* UsualArrayDeleteWantsArraySize: */ false,
        llvm::ArrayRef<clang::Expr *> { },
        clang::SourceRange { },
        /* arraySize: */ nullptr,
        clang::CXXNewExpr::InitializationStyle::CallInit,
        constructExpr,
        constructingType,
        /* AllocatedTypeInfo: */ nullptr,
        clang::SourceRange { },
        clang::SourceRange { });
    auto returnStmt = new (astContext) clang::ReturnStmt(
        clang::SourceLocation { },
        newExpr,
        /* NRVOCandidate: */ nullptr);

    std::vector<clang::Stmt *> bodyStatements { returnStmt };
    auto body = clang::CompoundStmt::Create(
        astContext,
        bodyStatements,
        clang::SourceLocation { },
        clang::SourceLocation { });

    wrapperFunc->setBody(body);
    return wrapperFunc;
  }

  /**
   * @brief Lookup the new operator available in the given declaration context.
   *
   * @param context the AST context.
   * @param declContext the declaration context.
   * @return clang::FunctionDecl* pointer to the function declaration of the new operator.
   */
  clang::FunctionDecl* lookupNewOperator(
      const clang::ASTContext& context, const clang::DeclContext* declContext) {
    auto declName = context.DeclarationNames.getCXXOperatorName(
        clang::OverloadedOperatorKind::OO_New);
    for (auto decl : declContext->lookup(declName)) {
      if (isDefaultNewOperator(decl)) {
        return llvm::dyn_cast<clang::FunctionDecl>(decl);
      }
    }

    return nullptr;
  }

  /**
   * @brief Determine whether the given named declaration is a default new operator.
   *
   * A named declaration is a default new operator if and only if:
   *
   * * decl is an instance of clang::FunctionDecl;
   * * The return type of decl is void*;
   * * The type of the only parameter of decl is std::size_t.
   *
   * @param decl The declaration to check.
   * @return true if the declaration is an instance of clang::FunctionDecl and it represents a
   * default new operator.
   * @return false otherwise.
   */
  bool isDefaultNewOperator(const clang::NamedDecl* decl) {
    if (!llvm::isa<clang::FunctionDecl>(decl)) {
      return false;
    }

    auto funcDecl = llvm::dyn_cast<clang::FunctionDecl>(decl);
    auto returnType = funcDecl->getReturnType().getTypePtr();
    if (!returnType->isVoidPointerType()) {
      return false;
    }

    if (funcDecl->getNumParams() != 1) {
      return false;
    }

    // The type of the first parameter can be guaranteed to be std::size_t by the semantic analyzer.

    return true;
  }

  /**
   * @brief Lookup the delete operator available in the given declaration context.
   *
   * @param context the AST context.
   * @param declContext the declaration context.
   * @return clang::FunctionDecl* pointer to the function declaration of the delete operator.
   */
  clang::FunctionDecl* lookupDeleteOperator(
      const clang::ASTContext& context, const clang::DeclContext* declContext) {
    auto declName = context.DeclarationNames.getCXXOperatorName(
        clang::OverloadedOperatorKind::OO_Delete);

    for (auto decl : declContext->lookup(declName)) {
      if (isDefaultDeleteOperator(decl)) {
        return llvm::dyn_cast<clang::FunctionDecl>(decl);
      }
    }

    return nullptr;
  }

  /**
   * @brief Determine whether the given named declaration is a default delete operator.
   *
   * A named declaration is a default delete operator if and only if:
   *
   * * decl is an instance of clang::FunctionDecl *;
   * * The return type of decl is void;
   * * The type of the only parameter of decl is void *.
   *
   * @param decl the named declaration to check.
   * @return true if the given declaration is a default delete oprator.
   * @return false if the given declaration is not a default delete operator.
   */
  bool isDefaultDeleteOperator(const clang::NamedDecl* decl) {
    if (!llvm::isa<clang::FunctionDecl>(decl)) {
      return false;
    }

    auto funcDecl = llvm::dyn_cast<clang::FunctionDecl>(decl);
    auto returnType = funcDecl->getReturnType().getTypePtr();
    if (!returnType->isVoidType()) {
      return false;
    }

    if (!funcDecl->getNumParams() != 1) {
      return false;
    }

    // The type of the only parameter given to delete operator is guaranteed by the semantic
    // analyzer.

    return true;
  }
}; // class CAFFindInlineConstructorVisitor

/**
 * @brief Implement an ASTConsumer instance that is used to wrap each constructor inside the
 * translation unit with a standalone exported function. CAF leverages these functions to activate
 * instances of classes.
 *
 */
class CAFConstructorWrapperASTConsumer : public clang::ASTConsumer {
public:
  /**
   * @brief Construct a new CAFConstructorWrapperASTConsumer object.
   *
   * @param compiler the compiler instance.
   */
  explicit CAFConstructorWrapperASTConsumer(clang::CompilerInstance& compiler)
    : _compiler(compiler),
      _visitor { compiler }
  { }

  /**
   * @brief Handle the translation unit.
   *
   * @param context clang::ASTContext instance containing the translation unit to process.
   */
  virtual void HandleTranslationUnit(clang::ASTContext& context) override {
    _visitor.TraverseDecl(context.getTranslationUnitDecl());
  }

private:
  clang::CompilerInstance& _compiler;
  CAFFindInlineConstructorVisitor _visitor;
}; // class CAFConstructorWrapperASTConsumer

/**
 * @brief Clang plugin for registering CAFConstructorWrapperASTConsumer into the list of AST
 * consumers.
 *
 */
class CAFConstructorWrapperAction : public clang::PluginASTAction {
protected:
  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
      clang::CompilerInstance& compiler,
      llvm::StringRef) override {
    return llvm::make_unique<CAFConstructorWrapperASTConsumer>(compiler);
  }

  bool ParseArgs(const clang::CompilerInstance &, const std::vector<std::string> &) override {
    // Nothing to do here.
    return true;
  }
}; // class CAFConstructorWrapperAction

} // namespace <anonymous>

static clang::FrontendPluginRegistry::Add<CAFConstructorWrapperAction> X(
    "caf-wrapper", "CAF constructor wrapper consumer");
