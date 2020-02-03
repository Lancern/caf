#include "CodeGen.h"
#include "SymbolTable.h"
#include "ABI.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"

#ifndef CAF_RECURSIVE_MAX_DEPTH
#define CAF_RECURSIVE_MAX_DEPTH 16
#endif

namespace caf {

namespace {

/**
 * @brief Determine whether instances of the given type can be allocated on the stack.
 *
 * @param type the type of the instance to be allocated.
 * @return true if instances of the given type can be allocated on the stack.
 * @return false if instances of the given type cannot be allocated on the stack.
 */
bool CanAlloca(const llvm::Type* type) {
  if (type->isStructTy()) {
    auto structType = llvm::dyn_cast<llvm::StructType>(type);
    return !structType->isOpaque();
  } else {
    return true;
  }
}

} // namespace anonymous

void CAFCodeGenerator::GenerateStub() {
  auto dispatchFunc = CreateDispatchFunction();

  //
  // =========== define dispatchFunc: the callee function for fuzzer. =============
  //

  llvm::Value* value_zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(
      _module->getContext()), 0);
  llvm::Value* value_one = llvm::ConstantInt::get(llvm::Type::getInt32Ty(
      _module->getContext()), 1);

  llvm::Value* int32Size = llvm::ConstantInt::get(
      llvm::Type::getInt32Ty(_module->getContext()), 1);

  //
  // ============= insert my new main function. =======================
  //

  // delete the old main function.
  auto oldMain = _module->getFunction("main");
  if (oldMain) {
    oldMain->eraseFromParent();
  }

  auto newMain = llvm::cast<llvm::Function>(
      _module->getOrInsertFunction(
          "main",
          llvm::IntegerType::getInt32Ty(_module->getContext()),
          llvm::IntegerType::getInt32Ty(_module->getContext()),
          llvm::PointerType::getUnqual(
              llvm::PointerType::getUnqual(
                  llvm::IntegerType::getInt8Ty(_module->getContext())
              )
          )
      )
  );

  {
    newMain->setCallingConv(llvm::CallingConv::C);
    auto args = newMain->arg_begin();
    (args++)->setName("argc");
    (args++)->setName("argv");
  }

  auto mainEntry = llvm::BasicBlock::Create(
      newMain->getContext(), "main.entry", newMain);
  auto mainWhileCond = llvm::BasicBlock::Create(
      newMain->getContext(), "main.while.cond", newMain);
  auto mainWhileBody = llvm::BasicBlock::Create(
      newMain->getContext(), "main.while.body", newMain);
  auto mainEnd = llvm::BasicBlock::Create(
      newMain->getContext(), "main.end", newMain);

  llvm::IRBuilder<> builder { newMain->getContext() };
  builder.SetInsertPoint(mainEntry);

  // input testcase size
  auto testCaseSize = builder.CreateAlloca(
      llvm::IntegerType::getInt32Ty(_module->getContext()),
      int32Size,
      "testcase_size");
  std::string scanfFormat("%d");
  CreateScanfCall(builder, scanfFormat, testCaseSize);

  builder.CreateBr(mainWhileCond);
  builder.SetInsertPoint(mainWhileCond);
  {
    llvm::Value* loadTestCaseSize = builder.CreateLoad(testCaseSize);
    llvm::Value* scanfCond = builder.CreateICmpSGT(loadTestCaseSize, value_zero);
    // llvm::Value* scanfCond = builder.CreateICmpNE(scanfRes, zero);
    builder.CreateCondBr(scanfCond, mainWhileBody, mainEnd);
  }

  builder.SetInsertPoint(mainWhileBody);
  {
    // testcase size -=1
    llvm::Value* loadTestCaseSize = builder.CreateLoad(testCaseSize);

    std::string printfFormat("%d\n");
    CreatePrintfCall(builder, printfFormat, loadTestCaseSize);

    auto testcaseSubone = builder.CreateSub(loadTestCaseSize, value_one);
    builder.CreateStore(testcaseSubone, testCaseSize);

    auto apiId = builder.CreateAlloca(
    llvm::IntegerType::getInt32Ty(_module->getContext()),
      int32Size,
      "api_id");
    std::string scanfFormat("%d");
    CreateScanfCall(builder, scanfFormat, apiId);

    auto loadApiId = builder.CreateLoad(apiId);
    CreatePrintfCall(builder, printfFormat, loadApiId);
    auto apiRetObj = builder.CreateCall(dispatchFunc, loadApiId);
  }
  builder.CreateBr(mainWhileCond);

  builder.SetInsertPoint(mainEnd);
  builder.CreateRet(value_zero);
}

llvm::CallInst* CAFCodeGenerator::CreatePrintfCall(
    llvm::IRBuilder<>& builder, const std::string& printfFormat, llvm::Value* value) {
  auto printfDecl = llvm::cast<llvm::Function>(
      _module->getOrInsertFunction(
          "printf",
          llvm::FunctionType::get(
              llvm::IntegerType::getInt32Ty(_module->getContext()),
              llvm::PointerType::getUnqual(
                  llvm::IntegerType::getInt8Ty(_module->getContext())
              ),
              true
          )
      )
  );
  printfDecl->setDSOLocal(true);

  llvm::Constant* stringConstant = llvm::ConstantDataArray::getString(
      builder.getContext(), printfFormat);
  llvm::Value* stringVar = builder.CreateAlloca(stringConstant->getType());
  builder.CreateStore(stringConstant, stringVar);
  llvm::Value* formatStr = builder.CreatePointerCast(stringVar, builder.getInt8PtrTy());
  llvm::Value* printfParams[] = { formatStr, value };
  return builder.CreateCall(printfDecl, printfParams);
}

llvm::CallInst* CAFCodeGenerator::CreatePrintfCall(
    llvm::IRBuilder<>& builder, const std::string& str) {
  auto printfDecl = llvm::cast<llvm::Function>(
      _module->getOrInsertFunction(
          "printf",
          llvm::FunctionType::get(
              llvm::IntegerType::getInt32Ty(_module->getContext()),
              llvm::PointerType::getUnqual(
                  llvm::IntegerType::getInt8Ty(_module->getContext())
              ),
              true
          )
      )
  );
  printfDecl->setDSOLocal(true);

  llvm::Constant* stringConstant = llvm::ConstantDataArray::getString(builder.getContext(), str);
  llvm::Value* stringVar = builder.CreateAlloca(stringConstant->getType());
  builder.CreateStore(stringConstant, stringVar);
  llvm::Value* formatStr = builder.CreatePointerCast(stringVar, builder.getInt8PtrTy());
  llvm::Value* printfParams[] = { formatStr };
  return builder.CreateCall(printfDecl, printfParams);
}

llvm::CallInst* CAFCodeGenerator::CreateScanfCall(
    llvm::IRBuilder<>& builder, const std::string& format, llvm::Value* dest) {
  auto scanfDecl = llvm::cast<llvm::Function>(
      _module->getOrInsertFunction(
          "scanf",
          llvm::FunctionType::get(
              llvm::IntegerType::getInt32Ty(_module->getContext()),
              llvm::PointerType::getUnqual(
                  llvm::IntegerType::getInt8Ty(_module->getContext())),
              true
          )
      )
  );
  scanfDecl->setDSOLocal(true);

  auto stringConstant = llvm::ConstantDataArray::getString(builder.getContext(), format);
  llvm::Value* stringVar = builder.CreateAlloca(stringConstant->getType());
  builder.CreateStore(stringConstant, stringVar);
  llvm::Value* formatStr = builder.CreatePointerCast(
      stringVar, builder.getInt8PtrTy());
  llvm::Value* scanfParams[] = { formatStr, dest };
  auto scanfRes = builder.CreateCall(scanfDecl, scanfParams);
  return scanfRes;
}

llvm::Function* CAFCodeGenerator::CreateDispatchFunction() {
  auto dispatchFunc = llvm::cast<llvm::Function>(
      _module->getOrInsertFunction(
          "__caf_dispatch",
          // llvm::Type::getInt64PtrTy(_module->getContext()),
          llvm::Type::getVoidTy(_module->getContext()),
          llvm::Type::getInt32Ty(_module->getContext())
      )
  );
  dispatchFunc->setCallingConv(llvm::CallingConv::C);

  auto argApiId = dispatchFunc->arg_begin();
  argApiId->setName("api");

  llvm::IRBuilder<> builder { dispatchFunc->getContext() };
  auto invokeApiEntry = llvm::BasicBlock::Create(
      dispatchFunc->getContext(),
      "caf.invoke.api.entry",
      dispatchFunc);

  std::vector<std::pair<llvm::ConstantInt *, llvm::BasicBlock *>> cases { };
  llvm::errs() << "apis num: " << _symbols->apis().size() << "\n";
  for (const auto& func: _symbols->apis()) {
    cases.push_back(CreateCallApiCase(func, dispatchFunc));
  }

  llvm::errs() << cases.size() << "\n";

  if (cases.size() == 0) {
    builder.SetInsertPoint(invokeApiEntry);
    builder.CreateRetVoid();
    return dispatchFunc;
  }

  auto invokeApiDefault = llvm::BasicBlock::Create(
      dispatchFunc->getContext(),
      "invoke.api.defualt",
      dispatchFunc);
  auto invokeApiEnd = llvm::BasicBlock::Create(
      dispatchFunc->getContext(),
      "invoke.api.end",
      dispatchFunc);

  for (auto& apiCase: cases) {
    auto bb = apiCase.second;
    // builder.SetInsertPoint(&*bb->end());
    builder.SetInsertPoint(bb);
    builder.CreateBr(invokeApiEnd);
  }

  builder.SetInsertPoint(invokeApiEntry);
  llvm::SwitchInst* invokeApiSI = builder.CreateSwitch(
      argApiId, cases.front().second, cases.size());
  for (auto& c: cases) {
    invokeApiSI->addCase(c.first, c.second);
  }
  invokeApiSI->setDefaultDest(invokeApiDefault);
  builder.SetInsertPoint(invokeApiDefault);
  builder.CreateBr(invokeApiEnd);
  builder.SetInsertPoint(invokeApiEnd);
  builder.CreateRetVoid();

  dispatchFunc->getType()->dump();
  return dispatchFunc;
}

llvm::Value* CAFCodeGenerator::AllocaStructValue(
    llvm::IRBuilder<>& builder, llvm::StructType* type, int depth) {
  llvm::Value* argAlloca = builder.CreateAlloca(type);
  auto stype = llvm::dyn_cast<llvm::StructType>(type);

  if (stype->isLiteral()) {
    std::string tname = type->getStructName().str();
    auto found = tname.find("."); //class.basename / struct.basename
    std::string basename = tname.substr(found + 1);

    // TODO: Refactor here to choosse different constructor.
    if (_symbols->GetConstructors(basename)) {
      auto ctor = (*_symbols->GetConstructors(basename))[0];
      std::vector<llvm::Value *> ctorParams { };
      llvm::Value* thisAlloca = nullptr;
      for (const auto& arg: ctor->args()) {
        if (!thisAlloca) {
          thisAlloca = argAlloca;
          ctorParams.push_back(thisAlloca);
          continue;
        }
        auto argType = arg.getType();
        auto argTypeAlloca = AllocaValueOfType(builder, argType, depth + 1);
        ctorParams.push_back(builder.CreateLoad(argTypeAlloca));
      }
      builder.CreateCall(ctor, ctorParams);
    } else {
      // TODO: Handle the situation if no constructors are found for the current struct type.
    }
  }

  return argAlloca;
}

llvm::Value* CAFCodeGenerator::AllocaPointerType(
    llvm::IRBuilder<>& builder, llvm::PointerType* type, int depth) {
  auto argAlloca = builder.CreateAlloca(type); // 按类型分配地址空间
  llvm::Type* pointeeType = type->getPointerElementType();

  if (depth >= CAF_RECURSIVE_MAX_DEPTH || !CanAlloca(pointeeType)) {
    auto nil = llvm::ConstantPointerNull::get(
        llvm::dyn_cast<llvm::PointerType>(type));
    builder.CreateStore(nil, argAlloca);
  } else {
    auto pointeeValue = AllocaValueOfType(builder, pointeeType, depth + 1);
    builder.CreateStore(pointeeValue, argAlloca);
  }

  return argAlloca;
}

llvm::Value* CAFCodeGenerator::AllocaArrayType(
    llvm::IRBuilder<>& builder, llvm::ArrayType* type, int depth) {
  // auto arraySize = llvm::ConstantInt::get(
  //     llvm::Type::getInt32Ty(builder.getContext()), type->getNumElements());
  // auto element_type = type->getElementType();
  // TODO: Add code here to allocate elements of the array.
  return builder.CreateAlloca(type);
}

llvm::Value* CAFCodeGenerator::AllocaFunctionType(
    llvm::IRBuilder<>& builder, llvm::FunctionType* type, int depth) {
  // TODO: Add code here to allocate a value of a function type.
  auto pointerType = type->getPointerTo();
  return builder.CreateLoad(builder.CreateAlloca(pointerType));
}

llvm::Value* CAFCodeGenerator::AllocaValueOfType(
    llvm::IRBuilder<>& builder, llvm::Type* type, int depth) {
  if (type->isIntegerTy() || type->isFloatingPointTy()) {
    return builder.CreateAlloca(type);
  } else if (type->isStructTy()) {
    return AllocaStructValue(builder, llvm::dyn_cast<llvm::StructType>(type), depth);
  } else if (type->isPointerTy()) {
    return AllocaPointerType(builder, llvm::dyn_cast<llvm::PointerType>(type), depth);
  } else if (type->isArrayTy()) {
    return AllocaArrayType(builder, llvm::dyn_cast<llvm::ArrayType>(type), depth);
  } else if (type->isFunctionTy()) {
    return AllocaFunctionType(builder, llvm::dyn_cast<llvm::FunctionType>(type), depth);
  } else {
    llvm::errs() << "Trying to alloca unrecognized type: "
        << type->getTypeID() << "\n";
    return nullptr;
  }
}

std::pair<llvm::ConstantInt *, llvm::BasicBlock *> CAFCodeGenerator::CreateCallApiCase(
    llvm::Function* callee, llvm::Function* caller) {
  llvm::IRBuilder<> builder { caller->getContext() };
  auto module = caller->getParent();
  auto& context = module->getContext();

  std::string block_name("invoke.api.case.");
  block_name.append(std::to_string(_apiCounter));
  auto invokeApiCase = llvm::BasicBlock::Create(
      caller->getContext(), block_name, caller);

  builder.SetInsertPoint(invokeApiCase);
  std::string printfFormat = demangle(callee->getName().str());
  printfFormat.push_back('\n');
  CreatePrintfCall(builder, printfFormat);

  std::vector<llvm::Value *> calleeArgs { };
  for (const auto& arg: callee->args()) {
    auto argAlloca = AllocaValueOfType(builder, arg.getType());
    auto argValue = builder.CreateLoad(argAlloca);

    calleeArgs.push_back(argValue);
  }
  builder.CreateCall(callee, calleeArgs);

  auto caseId = llvm::ConstantInt::get(
      llvm::Type::getInt32Ty(context), _apiCounter);
  _apiCounter++;
  return std::make_pair(caseId, invokeApiCase);
}

void CAFCodeGenerator::GenerateCallbackFunctionCandidateArray(
    const std::vector<Either<llvm::Function *, LLVMFunctionSignature>>& candidates) {
  // TODO: Implement CAFCodeGenerator::GenerateCallbackFunctionCandidateArray.
}

} // namespace caf
