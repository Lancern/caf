#include "CtorWrapperCodeGen.h"

#include "clang/AST/DeclarationName.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/Type.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/Specifiers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Sema/Sema.h"

#include "llvm/Support/Casting.h"

#include <cassert>
#include <vector>
#include <string>

#define CAF_WRAPPER_FUNC_NAME "CAFCreateObject"

namespace caf {

namespace {

class PopFunctionDeclContextGuard {
public:
  explicit PopFunctionDeclContextGuard(clang::Sema& sema)
    : _sema(sema)
  { }

  ~PopFunctionDeclContextGuard() {
    _sema.PopDeclContext();
    _sema.PopFunctionScopeInfo();
  }

private:
  clang::Sema& _sema;
}; // class PopFunctionDeclContextGuard

} // namespace <anonymous>

void CtorWrapperCodeGen::RegisterConstructor(clang::CXXConstructorDecl* ctor) {
  _ctors.insert(ctor->getCanonicalDecl());
}

void CtorWrapperCodeGen::GenerateCode() {
  for (auto ctor : _ctors) {
    auto wrapperFuncDecl = GenerateWrapperFuncMemberDeclFor(ctor);
    auto wrapperFuncDef = GenerateWrapperFuncDefinitionFor(ctor, wrapperFuncDecl);
    EmitLLVM(wrapperFuncDef);
  }
}

clang::CXXMethodDecl* CtorWrapperCodeGen::GenerateWrapperFuncMemberDeclFor(
    clang::CXXConstructorDecl* ctor) {
  auto record = ctor->getParent();

  auto method = CreateWrapperFuncDecl(ctor);
  method->setAccess(clang::AS_public);
  record->addDecl(method);

  return method;
}

clang::CXXMethodDecl* CtorWrapperCodeGen::GenerateWrapperFuncDefinitionFor(
    clang::CXXConstructorDecl* ctor, clang::CXXMethodDecl* decl) {
  auto def = CreateWrapperFuncDecl(ctor, _context.getTranslationUnitDecl());
  def->setPreviousDecl(decl);
  _context.getTranslationUnitDecl()->addDecl(def);

  GenerateWrapperFunctionBody(def, ctor);

  return def;
}

void CtorWrapperCodeGen::GenerateWrapperFunctionBody(
    clang::CXXMethodDecl* func, clang::CXXConstructorDecl* ctor) {
  auto record = ctor->getParent();
  auto recordType = _context.getRecordType(record);
  auto& sema = _compiler.getSema();

  // Jump to translation unit scope.
  while (sema.CurContext != _context.getTranslationUnitDecl()) {
    sema.PopDeclContext();
  }

  // sema.PushDeclContext(tuScope, func);
  sema.PushFunctionScope();
  sema.PushDeclContext(sema.getCurScope(), func);
  PopFunctionDeclContextGuard _popDeclContextGuard { sema };

  std::vector<clang::Expr *> constructArgs;
  constructArgs.reserve(ctor->getNumParams());
  for (size_t i = 0; i < func->getNumParams(); ++i) {
    constructArgs.push_back(GenerateConstructArgument(func, ctor, i));
  }

  auto constructExpr = clang::CXXConstructExpr::Create(
      /* const ASTContext& C = */ _context,
      /* QualType T = */ recordType,
      /* SourceLocation Loc = */ clang::SourceLocation { },
      /* CXXConstructorDecl* Ctor = */ ctor,
      /* bool Elidable = */ false,
      /* ArrayRef<Expr *> Args = */ constructArgs,
      /* bool HadMultipleCandidates = */ false,
      /* bool ListInitialization = */ false,
      /* bool StdInitListInitialization = */ false,
      /* bool ZeroInitialization = */ false,
      /* ConstructionKind ConstructKind = */ clang::CXXConstructExpr::CK_Complete,
      /* SourceRange ParenOrBranceRange = */ clang::SourceRange { });
  assert(constructExpr && "Create CXXConstrcutExpr failed.");

  // Clang hack: we need a valid DirectInitRange to make sema.BuildCXXNew think that the
  // initialization style is "DirectInitialization".
  auto& sourceManager = _compiler.getSourceManager();
  auto directInitLoc = sourceManager.getLocForEndOfFile(sourceManager.getMainFileID());
  auto newExpr = sema.BuildCXXNew(
      /* SourceRange Range = */ clang::SourceRange { },
      /* bool UseGlobal = */ false,
      /* SourceLocation PlacementLParen = */ clang::SourceLocation { },
      /* MultiExprArg PlacementArgs = */ clang::MultiExprArg { },
      /* SourceLocation PlacementRParen = */ clang::SourceLocation { },
      /* SourceRange TypeIdParens = */ clang::SourceRange { },
      /* QualType AllocType = */ recordType,
      /* TypeSourceInfo* AllocTypeInfo = */ _context.getTrivialTypeSourceInfo(recordType),
      /* Expr* ArraySize = */ nullptr,
      /* SourceRange DirectInitRange = */ directInitLoc,
      /* Expr* Initializer = */ constructExpr).get();
  assert(newExpr && "Create CXXNewExpr failed.");

  auto returnStmt = sema.BuildReturnStmt(
      /* SourceLocation ReturnLoc = */ clang::SourceLocation { },
      /* Expr* RetValExp = */ newExpr).get();
  assert(returnStmt && "Create ReturnStmt failed.");

  auto compoundStmt = clang::CompoundStmt::Create(
    /* ASTContext& context = */ _context,
    /* ArrayRef<Stmt *> Stmts = */ { returnStmt },
    /* SourceLocation LB = */ clang::SourceLocation { },
    /* SourceLocation RB = */ clang::SourceLocation { });
  assert(compoundStmt && "Create CompoundStmt failed.");

  func->setBody(compoundStmt);
}

clang::Expr* CtorWrapperCodeGen::GenerateConstructArgument(
    clang::CXXMethodDecl* func, clang::CXXConstructorDecl* ctor, size_t argIndex) {
  assert(argIndex < static_cast<size_t>(func->getNumParams()) &&
      "Invalid argument index.");
  assert(func->getNumParams() == ctor->getNumParams() &&
      "Numbers of parameters of the given function and constructor do not match.");

  auto& sema = _compiler.getSema();

  auto funcParamDecl = func->getParamDecl(argIndex);
  auto funcParamQualType = funcParamDecl->getType();
  auto ctorParamDecl = ctor->getParamDecl(argIndex);
  auto ctorParamQualType = ctorParamDecl->getType();
  auto ctorParamType = ctorParamQualType.getTypePtr();

  auto paramRefExprType = funcParamQualType;
  if (paramRefExprType->isReferenceType()) {
    paramRefExprType = llvm::cast<clang::ReferenceType>(paramRefExprType.getTypePtr())
        ->getPointeeType();
  }
  auto paramRefExpr = sema.BuildDeclRefExpr(
      /* ValueDecl* D = */ funcParamDecl,
      /* QualType Ty = */ paramRefExprType,
      /* ExprValueKind VK = */ clang::VK_LValue,
      /* SourceLocation Loc = */ clang::SourceLocation { }).get();
  assert(paramRefExpr && "Create DeclRefExpr failed.");

  if (ctorParamType->isStructureOrClassType() || ctorParamType->isRValueReferenceType()) {
    auto moveExprWrittenType = _context.getRValueReferenceType(funcParamQualType);
    auto moveExpr = clang::CXXStaticCastExpr::Create(
        /* const ASTContext& Context = */ _context,
        /* QualType T = */ funcParamQualType,
        /* ExprValueKind VK = */ clang::VK_XValue,
        /* CastKind K = */ clang::CK_LValueToRValue,
        /* Expr* Op = */ paramRefExpr,
        /* const CXXCastPath* Path = */ nullptr,
        /* TypeSourceInfo* Written = */ _context.getTrivialTypeSourceInfo(moveExprWrittenType),
        /* SourceLocation L = */ clang::SourceLocation { },
        /* SourceLocation RParenLoc = */ clang::SourceLocation { },
        /* SourceRange AngleBrackets = */ clang::SourceRange { });
    assert(moveExpr && "Create CXXStaticCastExpr failed.");

    if (ctorParamType->isRValueReferenceType()) {
      return moveExpr;
    }

    // ctorParamType->isStructureOrClassType() == true.

    auto ctorParamTypeClass = ctorParamType->getAsCXXRecordDecl();
    auto moveCtor = LookupMoveConstructor(ctorParamTypeClass);

    auto constructExpr = clang::CXXConstructExpr::Create(
      /* const ASTContext& C = */ _context,
      /* QualType T = */ ctorParamQualType,
      /* SourceLocation Loc = */ clang::SourceLocation { },
      /* CXXConstructorDecl* Ctor = */ moveCtor,
      /* bool Elidable = */ false,
      /* ArrayRef<Expr *> Args = */ { moveExpr },
      /* bool HadMultipleCandidates = */ false,
      /* bool ListInitialization = */ false,
      /* bool StdInitListInitialization = */ false,
      /* bool ZeroInitialization = */ false,
      /* ConstructionKind ConstructKind = */ clang::CXXConstructExpr::CK_Complete,
      /* SourceRange ParenOrBranceRange = */ clang::SourceRange { });
    assert(constructExpr && "Create CXXConstrcutExpr failed.");

    return constructExpr;
  } else if (ctorParamType->isLValueReferenceType()) {
    return paramRefExpr;
  } else {
    auto castExpr = clang::ImplicitCastExpr::Create(
        /* const ASTContext& Context = */ _context,
        /* QualType T = */ ctorParamQualType,
        /* CastKind Kind = */ clang::CK_LValueToRValue,
        /* Expr* Operand = */ paramRefExpr,
        /* const CXXCastPath* BasePath = */ nullptr,
        /* ExprValueKind Cat = */ clang::VK_RValue);
    assert(castExpr && "Create ImplicitCastExpr failed.");

    return castExpr;
  }
}

clang::CXXMethodDecl* CtorWrapperCodeGen::CreateWrapperFuncDecl(
    clang::CXXConstructorDecl* ctor, clang::DeclContext* lexicalContext) {
  auto funcQualType = GetWrapperFunctionType(ctor);
  auto funcName = GetDeclarationNameInfo(CAF_WRAPPER_FUNC_NAME);

  auto decl = clang::CXXMethodDecl::Create(
      /* ASTContext& C = */ _context,
      /* CXXRecordDecl* RD = */ ctor->getParent(),
      /* SourceLocation StartLoc = */ clang::SourceLocation { },
      /* const DeclarationNameInfo& NameInfo = */ funcName,
      /* QualType T = */ funcQualType,
      /* TypeSourceInfo* TInfo = */ _context.getTrivialTypeSourceInfo(funcQualType),
      /* StorageClass SC = */ clang::StorageClass::SC_Static,
      /* bool isInline = */ false,
      /* bool isConstexpr = */ false,
      /* SourceLocation EndLocation = */ clang::SourceLocation { });
  assert(decl && "Create CXXMethodDecl failed.");

  if (lexicalContext) {
    decl->setLexicalDeclContext(lexicalContext);
  }

  // Generate parameter declarations.
  auto funcType = llvm::cast<clang::FunctionProtoType>(funcQualType.getTypePtr());
  std::vector<clang::ParmVarDecl *> params;
  params.reserve(ctor->getNumParams());
  auto argIndex = 0;
  for (auto paramType : funcType->param_types()) {
    std::string argName = "_";
    argName.append(std::to_string(argIndex++));

    auto paramDecl = clang::ParmVarDecl::Create(
        /* ASTContext& C = */ _context,
        /* DeclContext* DC = */ decl,
        /* SourceLocation StartLoc = */ clang::SourceLocation { },
        /* SourceLocation IdLoc = */ clang::SourceLocation { },
        /* IdentifierInfo* Id = */ GetIdentifierInfo(argName.c_str()),
        /* QualType T = */ paramType,
        /* TypeSourceInfo* TInfo = */ _context.getTrivialTypeSourceInfo(paramType),
        /* StorageClass S = */ clang::StorageClass::SC_Auto,
        /* Expr* DefArg = */ nullptr);
    assert(paramDecl && "Create ParmVarDecl failed.");
    params.push_back(paramDecl);
  }

  decl->setParams(params);

  decl->addAttr(clang::WeakAttr::CreateImplicit(_context));

  return decl;
}

clang::QualType CtorWrapperCodeGen::GetWrapperFunctionType(clang::CXXConstructorDecl* ctor) {
  std::vector<clang::QualType> paramTypes;
  paramTypes.reserve(ctor->getNumParams());
  for (auto pr : ctor->parameters()) {
    paramTypes.push_back(pr->getType());
  }

  auto retType = _context.getPointerType(_context.getRecordType(ctor->getParent()));
  return _context.getFunctionType(retType, paramTypes, clang::FunctionProtoType::ExtProtoInfo { });
}

clang::IdentifierInfo* CtorWrapperCodeGen::GetIdentifierInfo(const char* name) {
  return &_compiler.getPreprocessor().getIdentifierTable().get(name);
}

clang::DeclarationName CtorWrapperCodeGen::GetDeclarationName(const char* name) {
  return clang::DeclarationName { GetIdentifierInfo(name) };
}

clang::DeclarationNameInfo CtorWrapperCodeGen::GetDeclarationNameInfo(const char* name) {
  return clang::DeclarationNameInfo { GetDeclarationName(name), clang::SourceLocation { } };
}

clang::CXXConstructorDecl* CtorWrapperCodeGen::LookupMoveConstructor(clang::CXXRecordDecl* record) {
  auto& sema = _compiler.getSema();
  for (auto ctor : record->ctors()) {
    if (sema.getSpecialMember(ctor) == clang::Sema::CXXMoveConstructor) {
      return ctor;
    }
  }

  auto ctor = llvm::cast<clang::CXXConstructorDecl>(sema.DeclareImplicitMoveConstructor(record));
  sema.DefineImplicitMoveConstructor(clang::SourceLocation { }, ctor);
  return ctor;
}

void CtorWrapperCodeGen::EmitLLVM(clang::CXXMethodDecl* decl) {
  _compiler.getASTConsumer().HandleTopLevelDecl(clang::DeclGroupRef { decl });
}

} // namespace caf
