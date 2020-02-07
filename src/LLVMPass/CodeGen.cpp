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
  auto ctors = _symbols->ctors();
  for(auto iter: ctors) {
    auto structTypeName = iter.first;
    CreateDispatchFunction(true, structTypeName);
  }

  auto dispatchFunc = CreateDispatchFunction(); 

  //
  // =========== define dispatchFunc: the callee function for fuzzer. =============
  //

  value_zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(
      _module->getContext()), 0);
  value_one = llvm::ConstantInt::get(llvm::Type::getInt32Ty(
      _module->getContext()), 1);

  int32Size = llvm::ConstantInt::get(
      llvm::Type::getInt32Ty(_module->getContext()), 1);

  int64Size = llvm::ConstantInt::get(
        llvm::Type::getInt64Ty(_module->getContext()), 1);

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
  CreateInputIntToCall(builder, testCaseSize);

  auto loadTestCaseSize = builder.CreateLoad(testCaseSize);

  auto mainWhileBr = builder.CreateBr(mainWhileCond);

  // llvm::Instruction* retObjList = llvm::CallInst::CreateMalloc(mainWhileBr, 
  //                          builder.getInt32Ty(),
  //                          builder.getInt64Ty(),
  //                          builder.getInt32(64), 
  //                          loadTestCaseSize, 
  //                          nullptr, 
  //                          "retobj_list");

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
    CreatePrintfCall(builder, "testcase num = %d\n", loadTestCaseSize);

    auto testcaseSubone = builder.CreateSub(loadTestCaseSize, value_one);
    builder.CreateStore(testcaseSubone, testCaseSize);

    auto apiId = builder.CreateAlloca(
      builder.getInt32Ty(),
      nullptr,
      "api_id");
    CreateInputIntToCall(builder, apiId);

    auto loadApiId = builder.CreateLoad(apiId);
    CreatePrintfCall(builder, "api_id = %d\n", loadApiId);
    auto apiRetObj = builder.CreateCall(dispatchFunc, loadApiId);

    // auto retObjPtr = builder.CreateGEP(apiRetObj, value_zero);
    // auto retObjSavePos = builder.CreateGEP(retObjList, testcaseSubone);
    // builder.CreateStore(retObjPtr, retObjSavePos);
  }
  builder.CreateBr(mainWhileCond);

  builder.SetInsertPoint(mainEnd);
  builder.CreateRet(value_zero);

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

llvm::CallInst* CAFCodeGenerator::CreateMallocCall(
  llvm::IRBuilder<>& builder, llvm::Value* size) {

    auto mallocFunc = llvm::cast<llvm::Function>(
        _module->getOrInsertFunction(
            "malloc",
            llvm::PointerType::getUnqual(
                llvm::IntegerType::getInt8Ty(_module->getContext())
            ),
            llvm::IntegerType::getInt32Ty(_module->getContext())
        )
    );
    auto sizeToInt32 = builder.CreateBitCast(
      size, llvm::IntegerType::getInt32Ty(_module->getContext()
    ));
    llvm::Value* params[] = { sizeToInt32 };
    return builder.CreateCall(mallocFunc, params, "malloccall");
}

llvm::CallInst* CAFCodeGenerator::CreateInputIntToCall(
  llvm::IRBuilder<>& builder, llvm::Value* dest) {

    auto inputIntToFunc = llvm::cast<llvm::Function>(
        _module->getOrInsertFunction(
            "inputIntTo",
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
            "inputBytesTo",
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
  }else {
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
      "caf.disaptch.entry",
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
      llvm::errs() << caseCounter << " cases have been created successfully.\n";
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
  llvm::Value* argAlloca = builder.CreateAlloca(type);
  unsigned TypeSize = _module->getDataLayout().getTypeAllocSize(type);
  if (llvm::StructType *ST = dynamic_cast<llvm::StructType*>(type))
    TypeSize = _module->getDataLayout().getStructLayout(type)->getSizeInBytes();
  // auto insertBefore = CreatePrintfCall(builder, "malloc for a struct type\n");
  // llvm::Value* argAlloca = llvm::CallInst::CreateMalloc(insertBefore,
  //                                               builder.getInt32Ty(),
  //                                               type,
  //                                               llvm::ConstantInt::get(builder.getInt32Ty(), TypeSize),
  //                                               nullptr,
  //                                               nullptr,
  //                                               "struct_malloc");
  
  // auto structMalloc = CreateMallocCall(builder, llvm::ConstantInt::get(builder.getInt32Ty(), TypeSize));
  // auto argAlloca = builder.CreateBitCast(structMalloc, type->getPointerTo());

                                             
  if(init == false)return argAlloca;

  auto inputKind = builder.CreateAlloca(
      llvm::IntegerType::getInt32Ty(_module->getContext()),
      nullptr,
      "input_kind");
  CreateInputIntToCall(builder, inputKind);
  auto inputKindValue = builder.CreateLoad(inputKind);
  CreatePrintfCall(builder, "input_kind = %d\n", inputKindValue);

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
    CreateMemcpyCall(builder, argAlloca, ctorSret, llvm::ConstantInt::get(builder.getInt64Ty(), TypeSize));
  } else {

  }

  return argAlloca;
}

llvm::Value* CAFCodeGenerator::AllocaPointerType(
    llvm::IRBuilder<>& builder, llvm::PointerType* type, int depth, bool init) {

  // unsigned TypeSize = _module->getDataLayout().getTypeAllocSize(type);
  // auto insertBefore = CreatePrintfCall(builder, "malloc for a pointer type\n");
  // llvm::Value* argAlloca = llvm::CallInst::CreateMalloc(insertBefore,
  //                                               builder.getInt32Ty(),
  //                                               type,
  //                                               llvm::ConstantInt::get(builder.getInt32Ty(), TypeSize),
  //                                               nullptr,
  //                                               nullptr,
  //                                               "pointer_malloc");
  auto argAlloca = builder.CreateAlloca(type); // 按类型分配地址空间
  // if(init == false)return argAlloca;

  llvm::Type* pointeeType = type->getPointerElementType();

  if (depth >= CAF_RECURSIVE_MAX_DEPTH || !CanAlloca(pointeeType)) {
    auto nil = llvm::ConstantPointerNull::get(
        llvm::dyn_cast<llvm::PointerType>(type));
    builder.CreateStore(nil, argAlloca);
  } else {
    auto inputKind = builder.CreateAlloca(
      llvm::IntegerType::getInt32Ty(_module->getContext()),
      nullptr,
      "input_kind");
    CreateInputIntToCall(builder, inputKind);
    auto inputKindValue = builder.CreateLoad(inputKind);
    CreatePrintfCall(builder, "input_kind = %d\n", inputKindValue);

    auto pointeeValue = AllocaValueOfType(builder, pointeeType, depth + 1, init);
    builder.CreateStore(pointeeValue, argAlloca);
  }

  return argAlloca;
}

llvm::Value* CAFCodeGenerator::AllocaArrayType(
    llvm::IRBuilder<>& builder, llvm::ArrayType* type, int depth, bool init) {
  // init = false;
  // TODO: Add code here to allocate elements of the array.
  // unsigned TypeSize = _module->getDataLayout().getTypeAllocSize(type);
  // auto insertBefore = CreatePrintfCall(builder, "malloc for a array type\n");
  // llvm::Value* arrayAddr = llvm::CallInst::CreateMalloc(insertBefore,
  //                                               builder.getInt32Ty(),
  //                                               type,
  //                                               llvm::ConstantInt::get(builder.getInt32Ty(), TypeSize),
  //                                               nullptr,
  //                                               nullptr,
  //                                               "array_malloc");
  auto arrayAddr = builder.CreateAlloca(type);

  if(init == true) {
    auto inputKind = builder.CreateAlloca(
        llvm::IntegerType::getInt32Ty(_module->getContext()),
        nullptr,
        "input_kind");
    CreateInputIntToCall(builder, inputKind);
    auto inputKindValue = builder.CreateLoad(inputKind);
    CreatePrintfCall(builder, "input_kind = %d\n", inputKindValue);

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
  // TODO: Add code here to allocate elements of the array.
  // unsigned TypeSize = _module->getDataLayout().getTypeAllocSize(type);
  // auto insertBefore = CreatePrintfCall(builder, "malloc for a vector type\n");
  // llvm::Value* vectorAddr = llvm::CallInst::CreateMalloc(insertBefore,
  //                                               builder.getInt32Ty(),
  //                                               type,
  //                                               llvm::ConstantInt::get(builder.getInt32Ty(), TypeSize),
  //                                               nullptr,
  //                                               nullptr,
  //                                               "vector_malloc");
  auto vectorAddr = builder.CreateAlloca(type);

  if(init == true) {
    auto inputKind = builder.CreateAlloca(
        llvm::IntegerType::getInt32Ty(_module->getContext()),
        nullptr,
        "input_kind");
    CreateInputIntToCall(builder, inputKind);
    auto inputKindValue = builder.CreateLoad(inputKind);
    CreatePrintfCall(builder, "input_kind = %d\n", inputKindValue);

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
  // unsigned TypeSize = _module->getDataLayout().getTypeAllocSize(pointerType);
  // auto insertBefore = CreatePrintfCall(builder, "malloc for a function type");
  // llvm::Value* argAlloca = llvm::CallInst::CreateMalloc(insertBefore,
  //                                               builder.getInt32Ty(),
  //                                               type,
  //                                               llvm::ConstantInt::get(builder.getInt32Ty(), TypeSize),
  //                                               nullptr,
  //                                               nullptr,
  //                                               "function_malloc");
  auto argAlloca = builder.CreateLoad(builder.CreateAlloca(pointerType));
  return argAlloca;
}

llvm::Value* CAFCodeGenerator::AllocaValueOfType(
    llvm::IRBuilder<>& builder, llvm::Type* type, int depth, bool init) {
      
  if (type->isIntegerTy() || type->isFloatingPointTy()) {
    auto valueAddr = builder.CreateAlloca(type);
    // unsigned TypeSize = _module->getDataLayout().getTypeAllocSize(type);
    // auto insertBefore = CreatePrintfCall(builder, "malloc for a int or float type\n");
    // llvm::Value* valueAddr = llvm::CallInst::CreateMalloc(insertBefore,
    //                                               builder.getInt32Ty(),
    //                                               type,
    //                                               llvm::ConstantInt::get(builder.getInt32Ty(), TypeSize),
    //                                               nullptr,
    //                                               nullptr,
    //                                               "int_or_float_malloc");
    if(init == true) {
      auto inputKind = builder.CreateAlloca(
          llvm::IntegerType::getInt32Ty(_module->getContext()),
          nullptr,
          "input_kind");
      CreateInputIntToCall(builder, inputKind);
      auto inputKindValue = builder.CreateLoad(inputKind);
      CreatePrintfCall(builder, "input_kind = %d\n", inputKindValue);

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
    return valueAddr;
  } else if (type->isStructTy()) {
    // return AllocaStructValue(builder, llvm::dyn_cast<llvm::StructType>(type), depth, init);
    return AllocaStructValue(builder, llvm::dyn_cast<llvm::StructType>(type), depth, init);
  } else if (type->isPointerTy()) {
    return AllocaPointerType(builder, llvm::dyn_cast<llvm::PointerType>(type), depth, init);
    // return AllocaPointerType(builder, llvm::dyn_cast<llvm::PointerType>(type), depth, false);
  } else if (type->isArrayTy()) {
    return AllocaArrayType(builder, llvm::dyn_cast<llvm::ArrayType>(type), depth, init);
  } else if(type->isVectorTy()) {
    return AllocaVectorType(builder, llvm::dyn_cast<llvm::VectorType>(type), depth, init);
  }else if (type->isFunctionTy()) {
    return AllocaFunctionType(builder, llvm::dyn_cast<llvm::FunctionType>(type), depth, init);
  } else {
    llvm::errs() << "Trying to alloca unrecognized type: "
        << type->getTypeID() << "\n";
    type->dump();
    return builder.CreateAlloca(type);
  }
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
    auto argValue = builder.CreateLoad(argAlloca);
    arg_num ++;

    calleeArgs.push_back(argValue);
  }
  auto retObj = builder.CreateCall(callee, calleeArgs);
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

} // namespace caf
