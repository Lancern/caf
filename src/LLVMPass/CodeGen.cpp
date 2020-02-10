#include "CodeGen.h"
#include "SymbolTable.h"
#include "ABI.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

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
  auto ctors = _symbols->ctors();
  for(auto iter: ctors) {
    auto structTypeName = iter.first;
    CreateDispatchFunction(true, structTypeName);
  }
  

  auto dispatchFunc = CreateDispatchFunction(); 
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

  auto callbackFuncArrDispatch = llvm::cast<llvm::Function>(
      _module->getOrInsertFunction(
          "__caf_dispatch_callbackfuncarr",
          builder.getVoidTy()
      )
  );
  builder.CreateCall(callbackFuncArrDispatch);

  // input testcase size
  auto testCaseSize = builder.CreateAlloca(
      builder.getInt32Ty(),
      nullptr,
      "testcase_size");
  CreateInputIntToCall(builder, testCaseSize);

  auto loadTestCaseSize = builder.CreateLoad(testCaseSize);

  auto mainWhileBr = builder.CreateBr(mainWhileCond);

  builder.SetInsertPoint(mainWhileCond);
  {
    llvm::Value* loadTestCaseSize = builder.CreateLoad(testCaseSize);
    llvm::Value* scanfCond = builder.CreateICmpSGT(loadTestCaseSize, builder.getInt32(0));
    // llvm::Value* scanfCond = builder.CreateICmpNE(scanfRes, zero);
    builder.CreateCondBr(scanfCond, mainWhileBody, mainEnd);
  }

  builder.SetInsertPoint(mainWhileBody);
  {
    // testcase size -=1
    llvm::Value* loadTestCaseSize = builder.CreateLoad(testCaseSize);
    CreatePrintfCall(builder, "testcase num = %d\n", loadTestCaseSize);

    auto testcaseSubone = builder.CreateSub(loadTestCaseSize, builder.getInt32(1));
    builder.CreateStore(testcaseSubone, testCaseSize);

    auto apiId = builder.CreateAlloca(
      builder.getInt32Ty(),
      nullptr,
      "api_id");
    CreateInputIntToCall(builder, apiId);

    auto loadApiId = builder.CreateLoad(apiId);
    CreatePrintfCall(builder, "api_id = %d\n", loadApiId);
    auto apiRetObj = builder.CreateCall(dispatchFunc, loadApiId);
  }
  builder.CreateBr(mainWhileCond);

  builder.SetInsertPoint(mainEnd);
  builder.CreateRet(builder.getInt32(0));

  llvm::errs() << "main has been created successfully.\n";
}

llvm::CallInst* CAFCodeGenerator::CreateMemcpyCall(
  llvm::IRBuilder<>& builder, llvm::Value* dest, llvm::Value*src, llvm::Value* size) {

    auto memcpyFunc = llvm::cast<llvm::Function>(
        _module->getOrInsertFunction(
            "llvm.memcpy.p0i8.p0i8.i64",
            builder.getVoidTy(),
            builder.getInt8PtrTy(),
            builder.getInt8PtrTy(),
            builder.getInt64Ty(),
            builder.getInt1Ty()
        )
    );
    auto destToI8Ptr = builder.CreateBitCast(
      dest, builder.getInt8PtrTy()
    );
    auto srcToI8Ptr = builder.CreateBitCast(
      src, builder.getInt8PtrTy()
    );
    auto sizeToInt64 = builder.CreateBitCast(
      size, llvm::IntegerType::getInt64Ty(_module->getContext()
    ));
    llvm::Value* params[] = { destToI8Ptr, srcToI8Ptr, sizeToInt64, builder.getInt1(false) };
    return builder.CreateCall(memcpyFunc, params);
}

llvm::Value* CAFCodeGenerator::CreateMallocCall(
  llvm::IRBuilder<>& builder, llvm::Type* type) {
    unsigned TypeSize = _module->getDataLayout().getTypeAllocSize(type);
    if (llvm::isa<llvm::StructType>(type)) {
      llvm::StructType *ST = llvm::cast<llvm::StructType>(type);
      TypeSize = _module->getDataLayout().getStructLayout(ST)->getSizeInBytes();
    }

    auto insertBefore = CreatePrintfCall(builder, "caf info: malloc for a type\n");
    llvm::Value* valueAddr = llvm::CallInst::CreateMalloc(insertBefore,
                                                  builder.getInt32Ty(),
                                                  type,
                                                  llvm::ConstantInt::get(builder.getInt32Ty(), TypeSize),
                                                  nullptr,
                                                  nullptr,
                                                  "malloc");
    insertBefore->eraseFromParent();
    return valueAddr;
}

llvm::CallInst* CAFCodeGenerator::CreateInputIntToCall(
  llvm::IRBuilder<>& builder, llvm::Value* dest) {

    auto inputIntToFunc = llvm::cast<llvm::Function>(
        _module->getOrInsertFunction(
            "_Z10inputIntToPi",
            llvm::Type::getVoidTy(_module->getContext()),
            llvm::PointerType::getUnqual(
                llvm::IntegerType::getInt32Ty(_module->getContext())
            )
        )
    );
    auto destToInt32Ptr = builder.CreateBitCast(
      dest, 
      llvm::PointerType::getUnqual(
        llvm::IntegerType::getInt32Ty(_module->getContext())
    ));
    llvm::Value* params[] = { destToInt32Ptr };
    return builder.CreateCall(inputIntToFunc, params);
}

llvm::CallInst* CAFCodeGenerator::CreateInputBtyesToCall(
  llvm::IRBuilder<>& builder, llvm::Value* dest, llvm::Value* size) {

    auto inputBytesToFunc = llvm::cast<llvm::Function>(
        _module->getOrInsertFunction(
            "_Z12inputBytesToPci",
            llvm::Type::getVoidTy(_module->getContext()),
            llvm::PointerType::getUnqual(
                llvm::IntegerType::getInt8Ty(_module->getContext())
            ),
            llvm::IntegerType::getInt32Ty(_module->getContext())
        )
    );
    auto destToInt8Ptr = builder.CreateBitCast(
      dest, 
      llvm::PointerType::getUnqual(
        llvm::IntegerType::getInt8Ty(_module->getContext())
    ));
    llvm::Value* params[] = { destToInt8Ptr, size };
    return builder.CreateCall(inputBytesToFunc, params);
}

llvm::CallInst* CAFCodeGenerator::CreateSaveToObjectListCall(
  llvm::IRBuilder<>& builder, llvm::Value* objPtr) {

    auto saveToObjectListFunc = llvm::cast<llvm::Function>(
        _module->getOrInsertFunction(
            "_Z16saveToObjectListl",
            builder.getVoidTy(),
            builder.getInt64Ty()
        )
    );
    auto objPtrToInt64 = builder.CreateBitCast(
      objPtr,
      builder.getInt64Ty()
    );
    llvm::Value* params[] = { objPtrToInt64 };
    return builder.CreateCall(saveToObjectListFunc, params);
}

llvm::CallInst* CAFCodeGenerator::CreateGetFromObjectListCall(
  llvm::IRBuilder<>& builder, llvm::Value* objIdx) {

    auto getFromObjectListFunc = llvm::cast<llvm::Function>(
        _module->getOrInsertFunction(
            "_Z17getFromObjectListi",
            builder.getInt64Ty(),
            builder.getInt32Ty()
        )
    );
    auto objIdxToInt32 = builder.CreateBitCast(
      objIdx,
      builder.getInt32Ty()
    );
    llvm::Value* params[] = { objIdxToInt32 };
    return builder.CreateCall(getFromObjectListFunc, params);
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

llvm::Function* CAFCodeGenerator::CreateDispatchFunction(
  bool ctorDispatch, std::string structTypeName) {
  std::string dispatchType("api");
  if(ctorDispatch == true)dispatchType = std::string("ctor");
  
  llvm::Function* dispatchFunc;
  if(ctorDispatch == false) {
    dispatchFunc = llvm::cast<llvm::Function>(
      _module->getOrInsertFunction(
          "__caf_dispatch_api",
          llvm::Type::getInt8PtrTy(_module->getContext()),
          // llvm::Type::getVoidTy(_module->getContext()),
          llvm::Type::getInt32Ty(_module->getContext())
      )
    );
  } else {
    dispatchFunc = llvm::cast<llvm::Function>(
      _module->getOrInsertFunction(
          "__caf_dispatch_ctor_" + structTypeName,
          llvm::Type::getInt8PtrTy(_module->getContext()),
          // llvm::Type::getVoidTy(_module->getContext()),
          llvm::Type::getInt32Ty(_module->getContext())
      )
    );
  }
  dispatchFunc->setCallingConv(llvm::CallingConv::C);

  auto argApiId = dispatchFunc->arg_begin();
  argApiId->setName(dispatchType);

  llvm::IRBuilder<> builder { dispatchFunc->getContext() };
  auto invokeApiEntry = llvm::BasicBlock::Create(
      dispatchFunc->getContext(),
      "caf.dispatch.entry",
      dispatchFunc);

  std::vector<std::pair<llvm::ConstantInt *, llvm::BasicBlock *>> cases { };
  if(ctorDispatch == false) {
    llvm::errs() << "apis num: " << _symbols->apis().size() << "\n";
    int caseCounter = 0;
    for (const auto& func: _symbols->apis()) {
      if(func->isIntrinsic()){
        llvm::errs() << "CodeGen.cpp: this is a intrinsic function: " << func->getName() << "\n";
        continue;
      }
      cases.push_back(CreateCallApiCase(func, dispatchFunc, caseCounter++));
      // llvm::errs() << caseCounter << " cases have been created successfully.\n";
    }
  } else {
    auto ctors = _symbols->GetConstructors(structTypeName);
    int caseCounter = 0;
    for (const auto& ctor: *ctors) {
      cases.push_back(CreateCallApiCase(ctor, dispatchFunc, caseCounter++, true));
    }
  }
  llvm::errs() << dispatchFunc->getName().str() << " - callee num: " << cases.size() << "\n";

  if (cases.size() == 0) {
    builder.SetInsertPoint(invokeApiEntry);
    {
    auto retObjPtr = builder.CreateAlloca(builder.getInt8Ty());
    builder.CreateStore(builder.getInt8(0), retObjPtr);
    builder.CreateRet(retObjPtr);
    }
    return dispatchFunc;
  }

  auto invokeApiDefault = llvm::BasicBlock::Create(
      dispatchFunc->getContext(),
      dispatchType + ".invoke.defualt",
      dispatchFunc);
  auto invokeApiEnd = llvm::BasicBlock::Create(
      dispatchFunc->getContext(),
      dispatchType + ".invoke.end",
      dispatchFunc);

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
  // if(ctorDispatch == true) 
  {
    auto retObjPtr = builder.CreateAlloca(builder.getInt8Ty());
    builder.CreateStore(builder.getInt8(0), retObjPtr);
    builder.CreateRet(retObjPtr);
  }
  llvm::errs() << "dispatchFunc created successfully.\n";
  return dispatchFunc;
}

llvm::Value* CAFCodeGenerator::AllocaStructValue(
    llvm::IRBuilder<>& builder, llvm::StructType* type, int depth, bool init) {
  // init = false;
  // llvm::Value* argAlloca = builder.CreateAlloca(type);
  llvm::Value* argAlloca = CreateMallocCall(builder, type);
                           
  if(init == false)return argAlloca;

  auto ctorId = builder.CreateAlloca(
    llvm::IntegerType::getInt32Ty(_module->getContext()),
    nullptr,
    "ctor_id");
  CreateInputIntToCall(builder, ctorId);
  auto ctorIdValue = dynamic_cast<llvm::Value *>(builder.CreateLoad(ctorId));
  CreatePrintfCall(builder, "ctor id = %d\n", ctorIdValue);

  auto stype = llvm::dyn_cast<llvm::StructType>(type);

  std::string tname = type->getStructName().str();
  auto found = tname.find("."); //class.basename / struct.basename
  std::string basename = tname.substr(found + 1);
  // llvm::errs() << "asked struct type: " << tname << "\n";

  auto dispatchFunc = _module->getFunction(
    "__caf_dispatch_ctor_" + tname
  );
  if(dispatchFunc) {
    // llvm::errs() << "!!!: " << dispatchFunc->getName().str() << "\n";
    llvm::Value* params[] = { ctorIdValue };
    auto ctorSret = builder.CreateCall(dispatchFunc, params);
    // auto ctorSretPtr = builder.CreateBitCast(ctorSret, type->getPointerTo());
    // auto ctorSretValue = builder.CreateLoad(ctorSretPtr);
    // builder.CreateStore(ctorSretValue, argAlloca);
    unsigned TypeSize = _module->getDataLayout().getTypeAllocSize(type);
    CreateMemcpyCall(builder, argAlloca, ctorSret, llvm::ConstantInt::get(builder.getInt64Ty(), TypeSize));
  } else {

  }

  return argAlloca;
}

llvm::Value* CAFCodeGenerator::AllocaPointerType(
    llvm::IRBuilder<>& builder, llvm::PointerType* type, int depth, bool init) {
  
  llvm::Value* argAlloca = CreateMallocCall(builder, type);
  // auto argAlloca = builder.CreateAlloca(type); // 按类型分配地址空间
  // if(init == false)return argAlloca;

  llvm::Type* pointeeType = type->getPointerElementType();

  if (depth >= CAF_RECURSIVE_MAX_DEPTH || !CanAlloca(pointeeType)) {
    auto nil = llvm::ConstantPointerNull::get(
        llvm::dyn_cast<llvm::PointerType>(type));
    builder.CreateStore(nil, argAlloca);
  } else {
    auto pointeeValue = AllocaValueOfType(builder, pointeeType, depth + 1, init);
    builder.CreateStore(pointeeValue, argAlloca);
  }

  return argAlloca;
}

llvm::Value* CAFCodeGenerator::AllocaArrayType(
    llvm::IRBuilder<>& builder, llvm::ArrayType* type, int depth, bool init) {

  llvm::Value* arrayAddr = CreateMallocCall(builder, type);
  // auto arrayAddr = builder.CreateAlloca(type);

  if(init == true) {
    auto arraySize = builder.CreateAlloca(
      llvm::IntegerType::getInt32Ty(_module->getContext()),
      nullptr,
      "array_size");
    CreateInputIntToCall(builder, arraySize);
    auto arraySizeValue = dynamic_cast<llvm::Value *>(builder.CreateLoad(arraySize));
    CreatePrintfCall(builder, "array size = %d\n", arraySizeValue);

    uint64_t arrsize = type->getArrayNumElements();
    llvm::Type * elementType = type->getArrayElementType();
    uint64_t elementSize = elementType->getScalarSizeInBits();
    for(uint64_t i = 0; i < arrsize; i++)
    {
      llvm::Value* element = AllocaValueOfType(builder, elementType, 0, init);
      llvm::Value* elementValue = builder.CreateLoad(element);
      llvm::Value *GEPIndices[] = { builder.getInt32(0), builder.getInt32(i) };
      llvm::Value * curAddr = builder.CreateInBoundsGEP(arrayAddr, GEPIndices);
      builder.CreateStore(elementValue, curAddr);
    }
  }

  return arrayAddr;
}

llvm::Value* CAFCodeGenerator::AllocaVectorType(
    llvm::IRBuilder<>& builder, llvm::VectorType* type, int depth, bool init) {
  // init = false;
  llvm::Value* vectorAddr = CreateMallocCall(builder, type);
  // auto vectorAddr = builder.CreateAlloca(type);

  if(init == true) {
    auto vectorSize = builder.CreateAlloca(
      llvm::IntegerType::getInt32Ty(_module->getContext()),
      nullptr,
      "vector_size");
    CreateInputIntToCall(builder, vectorSize);
    auto vectorSizeValue = dynamic_cast<llvm::Value *>(builder.CreateLoad(vectorSize));
    CreatePrintfCall(builder, "array size = %d\n", vectorSizeValue);

    uint64_t vecsize = type->getVectorNumElements();
    llvm::Type * elementType = type->getVectorElementType();
    uint64_t elementSize = elementType->getScalarSizeInBits();
    for(uint64_t i = 0; i < vecsize; i++)
    {
      llvm::Value* element = AllocaValueOfType(builder, elementType, 0, init);
      llvm::Value* elementValue = builder.CreateLoad(element);
      llvm::Value *GEPIndices[] = { builder.getInt32(0), builder.getInt32(i) };
      llvm::Value * curAddr = builder.CreateInBoundsGEP(vectorAddr, GEPIndices);
      builder.CreateStore(elementValue, curAddr);
    }
  }

  return vectorAddr;
}

llvm::Value* CAFCodeGenerator::AllocaFunctionType(
    llvm::IRBuilder<>& builder, llvm::FunctionType* type, int depth, bool init) {
  // TODO: Add code here to allocate a value of a function type.
  auto pointerType = type->getPointerTo();
  // auto pointerAlloca = builder.CreateAlloca(pointerType);
  llvm::Value* pointerAlloca = CreateMallocCall(builder, pointerType);
  if(init == true) {
    auto funcId = builder.CreateAlloca(
      llvm::IntegerType::getInt32Ty(_module->getContext()),
      nullptr,
      "func_id");
    CreateInputIntToCall(builder, funcId);
    auto funcIdValue = dynamic_cast<llvm::Value *>(builder.CreateLoad(funcId));
    CreatePrintfCall(builder, "func_id = %d\n", funcIdValue);

    auto callbackFuncArray = llvm::cast<llvm::Value>(
                            _module->getOrInsertGlobal("callbackfunc_candidates", 
                              llvm::ArrayType::get(
                                builder.getInt64Ty(),
                                callbackFunctionCandidatesNum
                              )
                          ));
    llvm::Value* arrIndex[] = { builder.getInt32(0), funcIdValue };
    auto funcAddrToLoad = builder.CreateInBoundsGEP(callbackFuncArray, arrIndex );
    auto funcAsInt64 = builder.CreateLoad(funcAddrToLoad);
    auto func = builder.CreateIntToPtr(funcAsInt64, pointerType);
    builder.CreateStore(func, pointerAlloca);
  }
  auto argAlloca = builder.CreateLoad(pointerAlloca);
  return argAlloca;
}

llvm::Value* CAFCodeGenerator::AllocaValueOfType(
    llvm::IRBuilder<>& builder, llvm::Type* type, int depth, bool init) {

  // auto beginBlock = llvm::BasicBlock::Create(
  //     builder.getContext(), "begin.malloc.object", 
  //     builder.GetInsertBlock()->getParent());
  // builder.CreateBr(beginBlock);
  // builder.SetInsertPoint(beginBlock);

  llvm::Value* inputKindValue;
  if(init == true) {
    auto inputKind = builder.CreateAlloca(
          llvm::IntegerType::getInt32Ty(_module->getContext()),
          nullptr,
          "input_kind");
      CreateInputIntToCall(builder, inputKind);
      inputKindValue = builder.CreateLoad(inputKind);
      CreatePrintfCall(builder, "input_kind = %d\n", inputKindValue);
  } else {
    if(type->isIntegerTy() || type->isFloatingPointTy()) {
      inputKindValue = builder.getInt32(0);
    } else if(type->isPointerTy()) {
      inputKindValue = builder.getInt32(1);
    } else if(type->isArrayTy() || type->isVectorTy()) {
      inputKindValue = builder.getInt32(2);
    } else if(type->isStructTy()) {
      inputKindValue = builder.getInt32(3);
    } else if(type->isFunctionTy()) {
      inputKindValue = builder.getInt32(5);
    }
  }
  
  auto kindEQ4 = builder.CreateICmpEQ(inputKindValue, builder.getInt32(4), "kindEQ4");

  auto thenBlock = llvm::BasicBlock::Create(
      builder.getContext(), "reuse.object.then", 
      builder.GetInsertBlock()->getParent());
  auto elseBlock = llvm::BasicBlock::Create(
      builder.getContext(), "reuse.object.else",
      builder.GetInsertBlock()->getParent());
  auto afterBlock = llvm::BasicBlock::Create(
      builder.getContext(),"after.malloc.object",
      builder.GetInsertBlock()->getParent());
  
  builder.CreateCondBr(kindEQ4, thenBlock, elseBlock);

  builder.SetInsertPoint(afterBlock);
  auto phiObj = builder.CreatePHI(type->getPointerTo(), 2);

  builder.SetInsertPoint(thenBlock); 
  // {
    auto objectIdx = builder.CreateAlloca(
          llvm::IntegerType::getInt32Ty(_module->getContext()),
          nullptr,
          "object_idx");
    CreateInputIntToCall(builder, objectIdx);
    auto objectIdxValue = builder.CreateLoad(objectIdx);
    CreatePrintfCall(builder, "object_idx = %d\n", objectIdxValue);

    auto ObjPtrToInt64 = CreateGetFromObjectListCall(builder, objectIdxValue);
    auto thenRet = builder.CreateIntToPtr(ObjPtrToInt64, type->getPointerTo());
    builder.CreateBr(afterBlock);
    phiObj->addIncoming(thenRet, builder.GetInsertBlock());
  // }
  
  llvm::Value* elseRet;
  builder.SetInsertPoint(elseBlock);
  // {
    if (type->isIntegerTy() || type->isFloatingPointTy()) {
      llvm::Value* valueAddr = CreateMallocCall(builder, type);
      // insertBefore->eraseFromParent();
      if(init == true) { // input_kind = 0
        auto inputSize = builder.CreateAlloca(
          llvm::IntegerType::getInt32Ty(_module->getContext()),
          nullptr,
          "input_size");
        CreateInputIntToCall(builder, inputSize);
        auto inputSizeValue = dynamic_cast<llvm::Value *>(builder.CreateLoad(inputSize));
        CreatePrintfCall(builder, "input size = %d\n", inputSizeValue);

        CreateInputBtyesToCall(builder, valueAddr, inputSizeValue);
        CreatePrintfCall(builder, "input value = %d\n", builder.CreateLoad(valueAddr));
      }
      elseRet = valueAddr;
    } else if (type->isStructTy()) {  // input_kind = 1
      elseRet = AllocaStructValue(builder, llvm::dyn_cast<llvm::StructType>(type), depth, init);
    } else if (type->isPointerTy()) { // input_kind = 2
      elseRet = AllocaPointerType(builder, llvm::dyn_cast<llvm::PointerType>(type), depth, init);
    } else if (type->isArrayTy()) {  // input_kind = 3
      elseRet = AllocaArrayType(builder, llvm::dyn_cast<llvm::ArrayType>(type), depth, init);
    } else if(type->isVectorTy()) {  // input_kind = 3
      elseRet = AllocaVectorType(builder, llvm::dyn_cast<llvm::VectorType>(type), depth, init);
    } else if (type->isFunctionTy()) {  // input_kind = 5
      elseRet = AllocaFunctionType(builder, llvm::dyn_cast<llvm::FunctionType>(type), depth, init);
    } else {
      llvm::errs() << "Trying to alloca unrecognized type: "
          << type->getTypeID() << "\n";
      type->dump();
      elseRet = builder.CreateAlloca(type);
    }
    // builder.SetInsertPoint(elseBlock);
    builder.CreateBr(afterBlock);
    phiObj->addIncoming(elseRet, builder.GetInsertBlock());
  // }
  builder.SetInsertPoint(afterBlock);

  return phiObj;
}

std::pair<llvm::ConstantInt *, llvm::BasicBlock *> CAFCodeGenerator::CreateCallApiCase(
    llvm::Function* callee, llvm::Function* caller, int caseCounter, bool hasSret) {
  llvm::IRBuilder<> builder { caller->getContext() };
  auto module = caller->getParent();
  auto& context = module->getContext();

  std::string block_name("invoke.case.");
  block_name.append(std::to_string(caseCounter));
  auto invokeApiCase = llvm::BasicBlock::Create(
      caller->getContext(), block_name, caller);

  builder.SetInsertPoint(invokeApiCase);
  std::string printfFormat = demangle(callee->getName().str());
  printfFormat.push_back('\n');
  CreatePrintfCall(builder, printfFormat);
  // llvm::errs() << printfFormat;

  std::vector<llvm::Value *> calleeArgs { };
  int arg_num = 1;
  for (const auto& arg: callee->args()) {
    auto argAlloca = AllocaValueOfType(builder, arg.getType(), 0, arg_num == 1 && hasSret ? false: true); 
    CreateSaveToObjectListCall(builder, builder.CreatePtrToInt(argAlloca, builder.getInt64Ty()));

    auto argValue = builder.CreateLoad(argAlloca);
    arg_num ++;

    calleeArgs.push_back(argValue);
  }
  auto retObj = builder.CreateCall(callee, calleeArgs);

  auto retType = retObj->getType();
  if(retType == builder.getVoidTy()) {
    auto voidRetPtr = CreateMallocCall(builder, builder.getInt64Ty());
    builder.CreateStore(builder.getInt64(0), voidRetPtr);
    CreateSaveToObjectListCall(builder, builder.CreatePtrToInt(voidRetPtr, builder.getInt64Ty()));
  } else {
    auto retObjMalloc = CreateMallocCall(builder, retType);
    unsigned TypeSize = _module->getDataLayout().getTypeAllocSize(retType);
      if (llvm::isa<llvm::StructType>(retType)) {
        llvm::StructType *ST = llvm::cast<llvm::StructType>(retType);
        TypeSize = _module->getDataLayout().getStructLayout(ST)->getSizeInBytes();
      }
    builder.CreateStore(retObj, retObjMalloc);
    CreateSaveToObjectListCall(builder, builder.CreatePtrToInt(retObjMalloc, builder.getInt64Ty()));
  }

  llvm::Value* retObjPtr;
  if(hasSret == false) {
    auto retType = retObj->getType(); // retType->dump();
    // callee->dump();
    if(retType == builder.getVoidTy()) {
      retObjPtr = builder.CreateAlloca(builder.getInt8Ty());
      builder.CreateStore(builder.getInt8(0), retObjPtr);
    }else {
      retObjPtr = builder.CreateAlloca(retObj->getType());
      builder.CreateStore(retObj, retObjPtr);
      retObjPtr = builder.CreateBitCast(retObjPtr, builder.getInt8PtrTy());
    }
  } else {
    retObjPtr = builder.CreateBitCast(calleeArgs[0], builder.getInt8PtrTy());
  }

  builder.CreateRet(retObjPtr);

  auto caseId = llvm::ConstantInt::get(
      llvm::Type::getInt32Ty(context), caseCounter);
  return std::make_pair(caseId, invokeApiCase);
}

void CAFCodeGenerator::GenerateCallbackFunctionCandidateArray(
    const std::vector<Either<llvm::Function *, LLVMFunctionSignature>>& candidates) {
  llvm::IRBuilder<> builder { _module->getContext() };

  _module->getOrInsertGlobal("callbackfunc_candidates", 
    llvm::ArrayType::get(
      builder.getInt64Ty(),
      candidates.size()
    )
  );
  auto callbackFuncArray = _module->getNamedGlobal("callbackfunc_candidates");
  callbackFuncArray->setLinkage(llvm::GlobalValue::LinkageTypes::CommonLinkage);
  callbackFuncArray->setInitializer(
    llvm::ConstantArray::get(
      llvm::ArrayType::get(
        llvm::Type::getInt64Ty(_module->getContext()), 
        candidates.size()
      ), 
      builder.getInt64(0)
    ));

  auto callbackFuncArrDispatch = llvm::cast<llvm::Function>(
      _module->getOrInsertFunction(
          "__caf_dispatch_callbackfuncarr",
          builder.getVoidTy()
      )
  );
  auto EntryBlock = llvm::BasicBlock::Create(
      callbackFuncArrDispatch->getContext(), "entry", callbackFuncArrDispatch);
  auto EndBlock = llvm::BasicBlock::Create(
      callbackFuncArrDispatch->getContext(), "end", callbackFuncArrDispatch);

  builder.SetInsertPoint(EntryBlock);
  
  callbackFunctionCandidatesNum = candidates.size();
  llvm::errs() << "candidates.size: " << callbackFunctionCandidatesNum << "\n";
  int curId = 0;
  for(auto iter: candidates) {
    llvm::Function* func;
    if(iter.isLhs()) {
      func = dynamic_cast<llvm::Function*>(*iter.GetLhs());
    } else {
      auto funcSig = dynamic_cast<LLVMFunctionSignature*>(iter.GetRhs());
      func = generateEmptyFunctionWithSignature(funcSig);
    }
    // func->dump();
    auto funcToStore = builder.CreatePtrToInt(func, builder.getInt64Ty());
    llvm::Value* arrIndex[] = { builder.getInt32(0), builder.getInt32(curId) };
    auto curAddrToStore = builder.CreateGEP(callbackFuncArray, arrIndex );
    builder.CreateStore(funcToStore, curAddrToStore);
    curId ++;
  }

  builder.CreateBr(EndBlock);
  builder.SetInsertPoint(EndBlock);
  builder.CreateRetVoid();
}

static std::string getSignature(llvm::FunctionType *FTy) {
  std::string Sig;
  llvm::raw_string_ostream OS(Sig);
  OS << *FTy->getReturnType();
  for (llvm::Type *ParamTy : FTy->params())
    OS << "_" << *ParamTy;
  if (FTy->isVarArg())
    OS << "_...";
  Sig = OS.str();
  Sig.erase(llvm::remove_if(Sig, isspace), Sig.end());
  // When s2wasm parses .s file, a comma means the end of an argument. So a
  // mangled function name can contain any character but a comma.
  std::replace(Sig.begin(), Sig.end(), ',', '.');
  return Sig;
}

llvm::Function* CAFCodeGenerator::generateEmptyFunctionWithSignature(LLVMFunctionSignature* funcSignature) {
  llvm::IRBuilder<> builder { _module->getContext() };

  auto retType = const_cast<llvm::Type*>(funcSignature->retType());
  auto paramTypes = funcSignature->paramTypes();
  std::vector<llvm::Type*> params;
  for(auto param: paramTypes)
    params.push_back(const_cast<llvm::Type*>(param));

  llvm::FunctionType* funcType = llvm::FunctionType::get(retType, params, false);
  std::string funcName = "__caf_emptyfunc_" + getSignature(funcType);
  auto emptyFunc = llvm::cast<llvm::Function>(
      _module->getOrInsertFunction(
          funcName,
          funcType
      )
  );
  auto EntryBlock = llvm::BasicBlock::Create(
      emptyFunc->getContext(), "entry", emptyFunc);

  builder.SetInsertPoint(EntryBlock);
  if(retType == builder.getVoidTy()) {
    builder.CreateRetVoid();
  } else {
    auto retValueAddr = AllocaValueOfType(builder, retType, 0, false);
    auto retValue = builder.CreateLoad(retValueAddr);
    builder.CreateRet(retValue);
  }

  return emptyFunc;
}

} // namespace caf
