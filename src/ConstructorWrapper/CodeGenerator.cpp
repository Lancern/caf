#include "CodeGenerator.h"

#include "clang/AST/AST.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Sema/Sema.h"

namespace caf {

bool CodeGenerator::EmitWrapperFunction(clang::CXXConstructorDecl* ctor) {
  assert(ctor && "ctor is nullptr.");

  auto wrapperFunc = createConstructorWrapperFunction(ctor);
  if (!wrapperFunc) {
    return false;
  }

  return emitTopLevelDecl(wrapperFunc);
}

bool CodeGenerator::emitTopLevelDecl(clang::Decl* decl) {
  return _compiler.getASTConsumer().HandleTopLevelDecl(clang::DeclGroupRef { decl });
}

clang::FunctionDecl* CodeGenerator::createConstructorWrapperFunction(
    clang::CXXConstructorDecl* ctor) {
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
  // auto attr = new (astContext) clang::Attr(
  //     /* attr::Kind Kind: */ clang::attr::Kind::Weak,
  //     /* SourceRange R: */ clang::SourceRange { },
  //     /* unsigned SpellingListIndex: */ 0, // TODO: Modify this argument
  //     /* bool IsLateParsed: */ false);
  // wrapperFunc->addAttr(attr);

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

clang::DeclarationName CodeGenerator::getWrapperFunctionName(clang::CXXConstructorDecl* ctor) {
  auto& identifierTable = _compiler.getPreprocessor().getIdentifierTable();

  std::string name { };
  name.append("CAFCallCtor_");
  name.append(ctor->getParent()->getNameAsString());

  return clang::DeclarationName { &identifierTable.get(name) };
}

clang::QualType CodeGenerator::getWrapperFunctionType(const clang::CXXConstructorDecl* ctor) {
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

clang::FunctionDecl* CodeGenerator::populateWrapperFunctionBody(
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

} // namespace caf
