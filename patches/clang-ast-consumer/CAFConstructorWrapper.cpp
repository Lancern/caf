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
 * @brief Provide a generator that generates wrapper functions of constructors.
 *
 */
class CAFCodeGenerator {
public:
  /**
   * @brief Construct a new CAFCodeGenerator object.
   *
   * @param compiler the compiler instance.
   */
  explicit CAFCodeGenerator(clang::CompilerInstance& compiler)
    : _compiler(compiler)
  { }

  /**
   * @brief Emit a wrapper function for the given constructor.
   *
   * @param ctor the constructor.
   * @return true if the wrapper function has been succesfully emitted.
   * @return false if something goes wrong.
   */
  bool EmitWrapperFunction(clang::CXXConstructorDecl* ctor) {
    assert(ctor && "ctor is null.");

    auto wrapperFunc = createConstructorWrapperFunction(ctor);
    if (!wrapperFunc) {
      return false;
    }

    return emitTopLevelDecl(wrapperFunc);
  }

private:
  clang::CompilerInstance& _compiler;

  /**
   * @brief Emit the given top level declaration by invoking AST consumers with this declaration.
   *
   * @param decl the declaration to invoke.
   * @return true if the AST consumer successfully handled the declaration.
   * @return false otherwise.
   */
  bool emitTopLevelDecl(clang::Decl* decl) {
    return _compiler.getASTConsumer().HandleTopLevelDecl(clang::DeclGroupRef { decl });
  }

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
    auto& sema = _compiler.getSema();

    // auto wrapperContext = astContext.getTranslationUnitDecl();
    auto wrapperContext = sema.CurContext;
    auto wrapperContextScope = sema.getScopeForContext(wrapperContext);
    assert(wrapperContextScope && "wrapperContextScope is null!");

    auto wrapperFunctionType = getWrapperFunctionType(ctor);
    auto wrapperFunc = clang::FunctionDecl::Create(
        /* ASTContext& C: */ astContext,
        /* DeclContext* DC: :*/ wrapperContext,
        /* SourceLocation StartLoc: */ clang::SourceLocation { },
        /* SourceLocation NLoc: */ clang::SourceLocation { },
        /* DeclarationName N: */ getWrapperFunctionName(ctor),
        /* QualType T: */ wrapperFunctionType,
        /* TypeSourceInfo* TInfo: */ astContext.getTrivialTypeSourceInfo(wrapperFunctionType),
        /* StorageClass SC: */ clang::StorageClass::SC_None);

    // Add weak symbol attribute to the created wrapper function.
    // TODO: Add weak symbol attribute to the created wrapper function.
    auto attr = new (astContext) clang::Attr(
        /* attr::Kind Kind: */ clang::attr::Kind::Weak,
        /* SourceRange R: */ clang::SourceRange { },
        /* unsigned SpellingListIndex: */ 0, // TODO: Modify this argument
        /* bool IsLateParsed: */ false);
    wrapperFunc->addAttr(attr);

    clang::Scope wrapperFunctionScope {
        wrapperContextScope, clang::Scope::FnScope, sema.getDiagnostics() };
    sema.PushDeclContext(&wrapperFunctionScope, wrapperFunc);
    sema.PushFunctionScope();

    populateWrapperFunctionBody(wrapperFunc, ctor);
    wrapperContext->addDecl(wrapperFunc);
    llvm::errs() << "Wrapper function " << wrapperFunc->getNameAsString() << " added.\n";

    sema.PopFunctionScopeInfo();
    sema.PopDeclContext();
    return wrapperFunc;
  }

  /**
   * @brief Get a clang::DeclarationName value indicating the name of the wrapper function around
   * the given constructor.
   *
   * @param ctor the constructor being wrapped by the wrapper function.
   * @return clang::DeclarationName the name of the wrapper function.
   */
  clang::DeclarationName getWrapperFunctionName(clang::CXXConstructorDecl* ctor) {
    auto& identifierTable = _compiler.getPreprocessor().getIdentifierTable();

    std::string name { };
    name.append("CAFCallCtor_");
    name.append(ctor->getParent()->getNameAsString());

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
    auto& identifierTable = _compiler.getPreprocessor().getIdentifierTable();
    auto& sema = _compiler.getSema();

    // The function body of the wrapper function is a simple return new T(...) statement where T is
    // the type declaring the constructor given.
    auto ctorParent = ctor->getParent();

    // Populate ParamVarDecl values to the wrapper function for each of the arguments specified.
    std::vector<clang::ParmVarDecl *> paramVars;
    auto wrapperFuncProtoType =
        llvm::dyn_cast<clang::FunctionProtoType>(wrapperFunc->getFunctionType());
    auto argIndex = 0;
    for (auto arg : wrapperFuncProtoType->getParamTypes()) {
      std::string argIdentifier { };
      argIdentifier.push_back('_');
      argIdentifier.append(std::to_string(argIndex++));

      paramVars.push_back(clang::ParmVarDecl::Create(
          /* ASTContext& C: */ astContext,
          /* DeclContext* DC: */ wrapperFunc,
          /* SourceLocation StartLoc: */ clang::SourceLocation { },
          /* SourceLocation IdLoc: */ clang::SourceLocation { },
          /* IdentifierInfo* Id: */ &identifierTable.get(argIdentifier),
          /* QualType T: */ arg,
          /* TypeSourceInfo* TInfo: */ astContext.getTrivialTypeSourceInfo(arg),
          /* StorageClass S: */ clang::StorageClass::SC_Auto,
          /* Expr* DefArg: */ nullptr));
    }
    wrapperFunc->setParams(paramVars);

    // Make an array of clang::Expr * denoting the arguments list that will be passed to the wrapped
    // constructor. Each entry in this vector is a clang::ImplicitCastExpr that cast the
    // corresponding wrapper function parameter from lvalue to rvalue.
    std::vector<clang::Expr *> ctorParams;
    ctorParams.reserve(wrapperFunc->getNumParams());
    for (auto paramDecl : wrapperFunc->parameters()) {
      auto paramRef = sema.BuildDeclRefExpr(
          /* ValueDecl* D: */ paramDecl,
          /* QualType Ty: */ paramDecl->getType(),
          /* ExprValueKind VK: */ clang::ExprValueKind::VK_LValue,
          /* SourceLocation SC: */ clang::SourceLocation { }
      ).get();
      assert(paramRef && "paramRef is null");

      auto paramRefCast = clang::ImplicitCastExpr::Create(
          /* const ASTContext& Context: */ astContext,
          /* QualType T: */ paramDecl->getType(),
          /* CastKind Kind: */ clang::CastKind::CK_LValueToRValue,
          /* Expr* Operand: */ paramRef,
          /* const CXXCastPath* BasePath: */ nullptr,
          /* ExprValueKind Cat: */ clang::ExprValueKind::VK_RValue);
      assert(paramRefCast && "paramRefCat is null");

      ctorParams.push_back(paramRefCast);
    }

    auto constructingType = astContext.getRecordType(ctorParent);
    auto constructExpr = sema.BuildCXXConstructExpr(
        /* clang::SourceLocation ConstructLoc: */ clang::SourceLocation { },
        /* clang::QualType DeclInitType: */ constructingType,
        /* clang::CXXConstructorDecl *Constructor: */ ctor,
        /* bool Edidable: */ false,
        /* clang::MultiExprArg Exprs: */ ctorParams,
        /* bool HadMultipleCandidates: */ false,
        /* bool IsListInitialization: */ false,
        /* bool IsStdInitListInitialization: */ false,
        /* bool RequiresZeroInit: */ false,
        /* unsigned int ConstructKind: */ clang::CXXConstructExpr::CK_Complete,
        /* clang::SourceRange ParenRange: */ clang::SourceRange { }
    ).get();
    assert(constructExpr && "constructExpr is null");

    auto& sourceManager = _compiler.getSourceManager();
    auto directInitLoc = sourceManager.getLocForEndOfFile(sourceManager.getMainFileID());
    auto newExpr = sema.BuildCXXNew(
        /* SourceRange Range: */ clang::SourceRange { },
        /* bool UseGlobal: */ false,
        /* SourceLocation PlacementLParen: */ clang::SourceLocation { },
        /* MultiExprArg PlacementArgs: */ clang::MultiExprArg { },
        /* SourceLocation PlacementRParen: */ clang::SourceLocation { },
        /* SourceRange TypedIdParens: */ clang::SourceRange { },
        /* QualType AllocType: */ constructingType,
        /* TypeSourceInfo* AllocTypeInfo: */ astContext.getTrivialTypeSourceInfo(constructingType),
        /* Expr* ArraySize: */ nullptr,
        /* SourceRange DirectInitRange: */ clang::SourceRange { directInitLoc },
        /* Expr* Initializer: */ constructExpr
    ).get();
    assert(newExpr && "newExpr is null");

    auto returnStmt = sema.BuildReturnStmt(
        /* SourceLocation ReturnLoc: */ clang::SourceLocation { },
        /* Expr* RetValExp: */ newExpr).get();
    assert(returnStmt && "returnStmt is null");

    std::vector<clang::Stmt *> bodyStatements { returnStmt };
    auto body = clang::CompoundStmt::Create(
        /* const ASTContext& C: */ astContext,
        /* ArrayRef<Stmt *> Stmts: */ bodyStatements,
        /* SourceLocation LB: */ clang::SourceLocation { },
        /* SourceLocation RB: */ clang::SourceLocation { });

    wrapperFunc->setBody(body);
    return wrapperFunc;
  }
};

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
    : _codeGen { compiler }
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

    return _codeGen.EmitWrapperFunction(decl);
  } // bool VisitCXXConstructorDecl

private:
  CAFCodeGenerator _codeGen;
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
    llvm::errs() << "CAF plugin is handling translation unit\n";
    _visitor.TraverseDecl(context.getTranslationUnitDecl());
  }

  /**
   * @brief Handle the top level declaration.
   *
   * @param declGroup the top level declaration to handle.
   */
  // virtual bool HandleTopLevelDecl(clang::DeclGroupRef declGroup) override {
  //   for (auto decl : declGroup) {
  //     llvm::errs() << "CAF plugin is handling declaration";
  //     if (llvm::isa<clang::NamedDecl>(decl)) {
  //       llvm::errs() << " " << llvm::dyn_cast<clang::NamedDecl>(decl)->getNameAsString();
  //     }
  //     llvm::errs() << "\n";

  //     if (!_visitor.TraverseDecl(decl)) {
  //       return false;
  //     }
  //   }

  //   return true;
  // }

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
  /**
   * @brief Create an ASTConsumer instance corresponding to this plugin.
   *
   * @param compiler the compiler instance.
   * @return std::unique_ptr<clang::ASTConsumer> the created ASTConsumer instance.
   */
  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
      clang::CompilerInstance& compiler,
      llvm::StringRef) override {
    return llvm::make_unique<CAFConstructorWrapperASTConsumer>(compiler);
  }

  bool ParseArgs(const clang::CompilerInstance &, const std::vector<std::string> &) override {
    // Nothing to do here.
    return true;
  }

  /**
   * @brief Get the type of this action. This function returns `AddBeforeMainAction` to indicate
   * that this action should be executed before the CodeGen module so that additional stuffs
   * generated by this action (the wrapper functions) can be properly emitted to LLVM IR.
   *
   * @return clang::PluginASTAction::ActionType the action type of this action.
   */
  clang::PluginASTAction::ActionType getActionType() override {
    return clang::PluginASTAction::AddBeforeMainAction;
  }
}; // class CAFConstructorWrapperAction

} // namespace <anonymous>

static clang::FrontendPluginRegistry::Add<CAFConstructorWrapperAction> X(
    "caf-wrapper", "CAF constructor wrapper consumer");
