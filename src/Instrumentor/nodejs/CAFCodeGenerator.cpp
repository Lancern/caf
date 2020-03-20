#include "Extractor/ExtractorContext.h"
#include "Instrumentor/nodejs/CAFCodeGenerator.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <cxxabi.h>

#ifndef CAF_RECURSIVE_MAX_DEPTH
#define CAF_RECURSIVE_MAX_DEPTH 1
#endif

namespace caf {

namespace {

std::string demangle(const std::string& name) {
  auto demangled = abi::__cxa_demangle(name.c_str(), nullptr, nullptr, nullptr);
  if (!demangled) {
    // Demangling failed. Return the original name.
    return name;
  }

  std::string ret(demangled);
  free(demangled);

  return ret;
}

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
  CreateDispatchMallocValueOfType();
  auto dispatchFunc = CreateDispatchFunctionForApi();
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
    builder.CreateCall(dispatchFunc, loadApiId);
  }
  builder.CreateBr(mainWhileCond);

  builder.SetInsertPoint(mainEnd);
  builder.CreateRet(builder.getInt32(0));

  // add attribute: optnone unwind
  {
    llvm::AttrBuilder B;
    B.addAttribute(llvm::Attribute::NoInline);
    B.addAttribute(llvm::Attribute::OptimizeNone);
    B.addAttribute(llvm::Attribute::UWTable);
    B.addAttribute(llvm::Attribute::NoRecurse);
    newMain->addAttributes(llvm::AttributeList::FunctionIndex, B);
  }

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

    auto insertBefore = CreatePrintfCall(builder, "caf info: create a malloc call\n");
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

llvm::CallInst* CAFCodeGenerator::CreateInputDoubleToCall(
  llvm::IRBuilder<>& builder, llvm::Value* dest) {

    auto inputDoubleToFunc = llvm::cast<llvm::Function>(
        _module->getOrInsertFunction(
            "_Z13inputDoubleToPd",
            llvm::Type::getVoidTy(_module->getContext()),
            llvm::PointerType::getUnqual(
              builder.getDoubleTy()
            )
        )
    );
    auto destToDoublePtr = builder.CreateBitCast(
      dest,
      llvm::PointerType::getUnqual(
        builder.getDoubleTy()
    ));
    llvm::Value* params[] = { destToDoublePtr };
    return builder.CreateCall(inputDoubleToFunc, params);
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

llvm::CallInst* CAFCodeGenerator::CreateGetRandomBytesCall(
  llvm::IRBuilder<>& builder, int bytesSize) {

    auto getRandomBytesFunc = llvm::cast<llvm::Function>(
        _module->getOrInsertFunction(
            "_Z13getRandomBitsi",
            builder.getInt8PtrTy(),
            builder.getInt32Ty()
        )
    );
    auto bytesSizeValue = builder.getInt32(bytesSize);
    llvm::Value* params[] = { bytesSizeValue };
    return builder.CreateCall(getRandomBytesFunc, params);
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

llvm::Function* CAFCodeGenerator::CreateDispatchFunctionForApi() {
  std::string dispatchType("api");

  llvm::Function* dispatchFunc;
  dispatchFunc = llvm::cast<llvm::Function>(
    _module->getOrInsertFunction(
        "__caf_dispatch_api",
        llvm::Type::getInt8PtrTy(_module->getContext()),
        // llvm::Type::getVoidTy(_module->getContext()),
        llvm::Type::getInt32Ty(_module->getContext())
    )
  );
  dispatchFunc->setCallingConv(llvm::CallingConv::C);

  auto argApiId = dispatchFunc->arg_begin();
  argApiId->setName(dispatchType);

  llvm::IRBuilder<> builder { dispatchFunc->getContext() };
  auto invokeApiEntry = llvm::BasicBlock::Create(
      dispatchFunc->getContext(),
      "caf.dispatch.entry",
      dispatchFunc);

  std::vector<std::pair<llvm::ConstantInt *, llvm::BasicBlock *>> cases { };
  const auto& apis = _extraction->GetApiFunctions();
  llvm::errs() << "apis num: " << apis.size() << "\n";
  int caseCounter = 0;
  for (auto func: apis) {
    auto api = const_cast<llvm::Function *>(func);
    auto apiId = _extraction->GetApiFunctionId(func).take();
    // add attribute: optnone noinline
    {
      if(api->hasFnAttribute(llvm::Attribute::OptimizeForSize)) 
        api->removeFnAttr(llvm::Attribute::OptimizeForSize);
      llvm::AttrBuilder B;
      B.addAttribute(llvm::Attribute::NoInline);
      B.addAttribute(llvm::Attribute::OptimizeNone);
      api->addAttributes(llvm::AttributeList::FunctionIndex, B);
    }
    cases.push_back(CreateCallApiCase(
        api, dispatchFunc, apiId));
    // llvm::errs() << caseCounter << " cases have been created successfully.\n";
  }
  llvm::errs() << dispatchFunc->getName().str() << " - callee num: " << cases.size() << "\n";

  if (cases.size() == 0) {
    builder.SetInsertPoint(invokeApiEntry);
    {
    auto retObjPtr = builder.CreateAlloca(builder.getInt8Ty());
    builder.CreateStore(builder.getInt8(0), retObjPtr);
    builder.CreateRet(retObjPtr);
    }
    // return dispatchFunc;
  } else {
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
  }
  // add attribute: optnone unwind
  {
    llvm::AttrBuilder B;
    B.addAttribute(llvm::Attribute::NoInline);
    B.addAttribute(llvm::Attribute::OptimizeNone);
    B.addAttribute(llvm::Attribute::UWTable);
    dispatchFunc->addAttributes(llvm::AttributeList::FunctionIndex, B);
  }
  llvm::errs() << "api dispatchFunc created successfully.\n";
  return dispatchFunc;
}

// llvm::Value* CAFCodeGenerator::AllocaStructValue(
//     llvm::IRBuilder<>& builder, llvm::StructType* type, int depth, bool init) {
//   // init = false;
//   // llvm::Value* argAlloca = builder.CreateAlloca(type);
//   llvm::Value* argAlloca = CreateMallocCall(builder, type);
//   if(init) {
//     CreatePrintfCall(builder, "structType: ");
//     CreateSaveToObjectListCall(builder, builder.CreatePtrToInt(argAlloca, builder.getInt64Ty()));
//   }

//   if(init == false || depth >= CAF_RECURSIVE_MAX_DEPTH)return argAlloca;

//   auto ctorId = builder.CreateAlloca(
//     llvm::IntegerType::getInt32Ty(_module->getContext()),
//     nullptr,
//     "ctor_id");
//   CreateInputIntToCall(builder, ctorId);
//   auto ctorIdValue = dynamic_cast<llvm::Value *>(builder.CreateLoad(ctorId));
//   CreatePrintfCall(builder, "ctor_id = %d\n", ctorIdValue);

//   auto stype = llvm::dyn_cast<llvm::StructType>(type);

//   std::string tname("");
//   if(!type->isLiteral()) {
//     tname = type->getStructName().str();
//     auto found = tname.find("."); //class.basename / struct.basename
//     std::string basename = tname.substr(found + 1);
//   }
//   // llvm::errs() << "asked struct type: " << tname << "\n";

//   auto dispatchFunc = _module->getFunction(
//     "__caf_dispatch_ctor_" + tname
//   );
//   if(dispatchFunc) {
//     // llvm::errs() << "!!!: " << dispatchFunc->getName().str() << "\n";
//     llvm::Value* params[] = { ctorIdValue };
//     auto ctorSret = builder.CreateCall(dispatchFunc, params);
//     // auto ctorSretPtr = builder.CreateBitCast(ctorSret, type->getPointerTo());
//     // auto ctorSretValue = builder.CreateLoad(ctorSretPtr);
//     // builder.CreateStore(ctorSretValue, argAlloca);
//     unsigned TypeSize = _module->getDataLayout().getTypeAllocSize(type);
//     CreateMemcpyCall(builder, argAlloca, ctorSret, llvm::ConstantInt::get(builder.getInt64Ty(), TypeSize));
//   } else {
//     // llvm::errs() << tname << " has no ctors.\n\n";
//     int elementId = 0;
//     for(auto elementType: type->elements()) {
//       // llvm::errs() << tname << " elementId: " << elementId << "\n";
//       llvm::Value* element = AllocaValueOfType(builder, elementType, depth + 1, init);
//       llvm::Value* elementValue = builder.CreateLoad(element);
//       llvm::Value *GEPIndices[] = { builder.getInt32(0), builder.getInt32(elementId++) };
//       llvm::Value * curAddr = builder.CreateInBoundsGEP(argAlloca, GEPIndices);
//       builder.CreateStore(elementValue, curAddr);
//     }
//     // llvm::errs() << tname << " type created done.\n";
//   }

//   return argAlloca;
// }

// llvm::Value* CAFCodeGenerator::AllocaPointerType(
//     llvm::IRBuilder<>& builder, llvm::PointerType* type, int depth, bool init) {

//   llvm::Value* argAlloca = CreateMallocCall(builder, type);
  
//   if(init) {
//     CreatePrintfCall(builder, "pointerType: ");
//     CreateSaveToObjectListCall(builder, builder.CreatePtrToInt(argAlloca, builder.getInt64Ty()));
//   }
  
//   // auto argAlloca = builder.CreateAlloca(type); // 按类型分配地址空间
//   // if(init == false)return argAlloca;

//   llvm::Type* pointeeType = type->getPointerElementType();

//   if (depth >= CAF_RECURSIVE_MAX_DEPTH || !CanAlloca(pointeeType) || init == false) {
//     auto nil = llvm::ConstantPointerNull::get(
//         llvm::dyn_cast<llvm::PointerType>(type));
//     builder.CreateStore(nil, argAlloca);
//   } else {
//     auto pointeeValue = AllocaValueOfType(builder, pointeeType, depth + 1, init);
//     builder.CreateStore(pointeeValue, argAlloca);
//   }

//   return argAlloca;
// }

// llvm::Value* CAFCodeGenerator::AllocaArrayType(
//     llvm::IRBuilder<>& builder, llvm::ArrayType* type, int depth, bool init) {

//   llvm::Value* arrayAddr = CreateMallocCall(builder, type);
//   if(init) {
//     CreatePrintfCall(builder, "arrayType: ");
//     CreateSaveToObjectListCall(builder, builder.CreatePtrToInt(arrayAddr, builder.getInt64Ty()));
//   }
  
//   // auto arrayAddr = builder.CreateAlloca(type);

//   if(init == true && depth < CAF_RECURSIVE_MAX_DEPTH) {
//     auto arraySize = builder.CreateAlloca(
//       llvm::IntegerType::getInt32Ty(_module->getContext()),
//       nullptr,
//       "array_size");
//     CreateInputIntToCall(builder, arraySize);
//     auto arraySizeValue = dynamic_cast<llvm::Value *>(builder.CreateLoad(arraySize));
//     CreatePrintfCall(builder, "array size = %d\n", arraySizeValue);

//     uint64_t arrsize = type->getArrayNumElements();
//     llvm::Type * elementType = type->getArrayElementType();
//     uint64_t elementSize = elementType->getScalarSizeInBits();
//     for(uint64_t i = 0; i < arrsize; i++)
//     {
//       llvm::Value* element = AllocaValueOfType(builder, elementType, depth + 1, init);
//       llvm::Value* elementValue = builder.CreateLoad(element);
//       llvm::Value *GEPIndices[] = { builder.getInt32(0), builder.getInt32(i) };
//       llvm::Value * curAddr = builder.CreateInBoundsGEP(arrayAddr, GEPIndices);
//       builder.CreateStore(elementValue, curAddr);
//     }
//   }

//   return arrayAddr;
// }

// llvm::Value* CAFCodeGenerator::AllocaVectorType(
//     llvm::IRBuilder<>& builder, llvm::VectorType* type, int depth, bool init) {
//   // init = false;
//   llvm::Value* vectorAddr = CreateMallocCall(builder, type);
//   if(init) {
//     CreatePrintfCall(builder, "vectorType: ");
//     CreateSaveToObjectListCall(builder, builder.CreatePtrToInt(vectorAddr, builder.getInt64Ty()));
//   }
  
//   // auto vectorAddr = builder.CreateAlloca(type);

//   if(init == true && depth < CAF_RECURSIVE_MAX_DEPTH) {
//     auto vectorSize = builder.CreateAlloca(
//       llvm::IntegerType::getInt32Ty(_module->getContext()),
//       nullptr,
//       "vector_size");
//     CreateInputIntToCall(builder, vectorSize);
//     auto vectorSizeValue = dynamic_cast<llvm::Value *>(builder.CreateLoad(vectorSize));
//     CreatePrintfCall(builder, "array size = %d\n", vectorSizeValue);

//     uint64_t vecsize = type->getVectorNumElements();
//     llvm::Type * elementType = type->getVectorElementType();
//     uint64_t elementSize = elementType->getScalarSizeInBits();
//     for(uint64_t i = 0; i < vecsize; i++)
//     {
//       llvm::Value* element = AllocaValueOfType(builder, elementType, depth + 1, init);
//       llvm::Value* elementValue = builder.CreateLoad(element);
//       llvm::Value *GEPIndices[] = { builder.getInt32(0), builder.getInt32(i) };
//       llvm::Value * curAddr = builder.CreateInBoundsGEP(vectorAddr, GEPIndices);
//       builder.CreateStore(elementValue, curAddr);
//     }
//   }

//   return vectorAddr;
// }

// llvm::Value* CAFCodeGenerator::AllocaFunctionType(
//     llvm::IRBuilder<>& builder, llvm::FunctionType* type, int depth, bool init) {
//   // TODO: Add code here to allocate a value of a function type.
//   auto pointerType = type->getPointerTo();
//   // auto pointerAlloca = builder.CreateAlloca(pointerType);
//   llvm::Value* pointerAlloca = CreateMallocCall(builder, pointerType);
//   if(init == true && depth < CAF_RECURSIVE_MAX_DEPTH) {
//     auto funcId = builder.CreateAlloca(
//       llvm::IntegerType::getInt32Ty(_module->getContext()),
//       nullptr,
//       "func_id");
//     CreateInputIntToCall(builder, funcId);
//     auto funcIdValue = dynamic_cast<llvm::Value *>(builder.CreateLoad(funcId));
//     CreatePrintfCall(builder, "func_id = %d\n", funcIdValue);

//     auto callbackFuncArray = llvm::cast<llvm::Value>(
//                             _module->getOrInsertGlobal("callbackfunc_candidates",
//                               llvm::ArrayType::get(
//                                 builder.getInt64Ty(),
//                                 callbackFunctionCandidatesNum
//                               )
//                           ));
//     llvm::Value* arrIndex[] = { builder.getInt32(0), funcIdValue };
//     auto funcAddrToLoad = builder.CreateInBoundsGEP(callbackFuncArray, arrIndex );
//     auto funcAsInt64 = builder.CreateLoad(funcAddrToLoad);
//     auto func = builder.CreateIntToPtr(funcAsInt64, pointerType);
//     builder.CreateStore(func, pointerAlloca);
//   }
//   auto argAlloca = builder.CreateLoad(pointerAlloca);
//   if(init) {
//     CreatePrintfCall(builder, "functionType: ");
//     CreateSaveToObjectListCall(builder, builder.CreatePtrToInt(argAlloca, builder.getInt64Ty()));
//   }
  
//   return argAlloca;
// }


llvm::Value* CAFCodeGenerator::MallocUndefinedType(llvm::IRBuilder<> builder) {
  auto cafCreateUndefinedFunc = llvm::cast<llvm::Function>(
    _module->getOrInsertFunction(
      "_ZN9caf_v8lib19caf_CreateUndefinedEv", // caf_CreateUndefined
      builder.getInt8PtrTy()
    )
  );
  auto undefinedPtr = builder.CreateCall(cafCreateUndefinedFunc);
  return undefinedPtr;
}

llvm::Value* CAFCodeGenerator::MallocNullType(llvm::IRBuilder<> builder) {
  auto cafCreateNullFunc = llvm::cast<llvm::Function>(
    _module->getOrInsertFunction(
      "_ZN9caf_v8lib14caf_CreateNullEv", // caf_CreateNull
      builder.getInt8PtrTy()
    )
  );
  auto nullPtr = builder.CreateCall(cafCreateNullFunc);
  return nullPtr;
}

llvm::Value* CAFCodeGenerator::MallocStringType(llvm::IRBuilder<> builder) {
  auto cafCreateStringFunc = llvm::cast<llvm::Function>(
    _module->getOrInsertFunction(
      "_ZN9caf_v8lib16caf_CreateStringEPc", // caf_CreateString
      builder.getInt8PtrTy(),
      builder.getInt8PtrTy()
    )
  );

  auto inputStringLength = builder.CreateAlloca(
    llvm::IntegerType::getInt32Ty(_module->getContext()),
    nullptr,
    "input_string_length");
  CreateInputIntToCall(builder, inputStringLength);
  auto inputStringLengthValue = dynamic_cast<llvm::Value *>(builder.CreateLoad(inputStringLength));
  CreatePrintfCall(builder, "input_string_length = %d\n", inputStringLengthValue);
  auto valueAddr = builder.CreateAlloca(builder.getInt8PtrTy(), inputStringLengthValue, "input_string");

  CreateInputBtyesToCall(builder, valueAddr, inputStringLengthValue);
  CreatePrintfCall(builder, "input value = %d\n", builder.CreateLoad(valueAddr));
  
  auto stringPtr = builder.CreateCall(cafCreateStringFunc, { valueAddr } );
  return stringPtr;
}

llvm::Value* CAFCodeGenerator::MallocIntegerType(llvm::IRBuilder<> builder) {
  auto cafCreateIntegerFunc = llvm::cast<llvm::Function>(
    _module->getOrInsertFunction(
      "_ZN9caf_v8lib17caf_CreateIntegerEi", // caf_CreateInteger
      builder.getInt8PtrTy(),
      builder.getInt32Ty()
    )
  );
  auto inputInteger = builder.CreateAlloca(
    llvm::IntegerType::getInt32Ty(_module->getContext()),
    nullptr,
    "input_integer");
  CreateInputIntToCall(builder, inputInteger);
  auto inputIntegerValue = dynamic_cast<llvm::Value *>(builder.CreateLoad(inputInteger));
  CreatePrintfCall(builder, "input integer = %d\n", inputIntegerValue);
  // llvm::Value* params[] = {};
  auto integerPtr = builder.CreateCall(cafCreateIntegerFunc, { inputIntegerValue } );
  return integerPtr;
}

llvm::Value* CAFCodeGenerator::MallocBooleanType(llvm::IRBuilder<> builder) {
  auto cafCreateIntegerFunc = llvm::cast<llvm::Function>(
    _module->getOrInsertFunction(
      "_ZN9caf_v8lib17caf_CreateBooleanEb", // caf_CreateBoolean
      builder.getInt8PtrTy(),
      builder.getInt8Ty()
    )
  );
  auto inputBoolean = builder.CreateAlloca(
    llvm::IntegerType::getInt8Ty(_module->getContext()),
    nullptr,
    "input_boolean");
  CreateInputBtyesToCall(builder, inputBoolean, builder.getInt32(1));
  auto inputBooleanValue = dynamic_cast<llvm::Value *>(builder.CreateLoad(inputBoolean));
  CreatePrintfCall(builder, "input boolean = %hhx\n", inputBooleanValue);
  // llvm::Value* params[] = {};
  auto booleanPtr = builder.CreateCall(cafCreateIntegerFunc, { inputBooleanValue } );
  return booleanPtr;
}

llvm::Value* CAFCodeGenerator::MallocFloatingPointerType(llvm::IRBuilder<> builder) {
  auto cafCreateDoubleFunc = llvm::cast<llvm::Function>(
    _module->getOrInsertFunction(
      "_ZN9caf_v8lib16caf_CreateNumberEd", // caf_CreateNumber
      builder.getInt8PtrTy(),
      builder.getDoubleTy()
    )
  );
  auto inputDouble = builder.CreateAlloca(
    builder.getDoubleTy(),
    nullptr,
    "input_double");
  CreateInputDoubleToCall(builder, inputDouble);
  auto inputDoubleValue = dynamic_cast<llvm::Value *>(builder.CreateLoad(inputDouble));
  CreatePrintfCall(builder, "input double = %lf\n", inputDoubleValue);
  // llvm::Value* params[] = {};
  auto doublePtr = builder.CreateCall(cafCreateDoubleFunc, { inputDoubleValue } );
  return doublePtr;
}

llvm::Value* CAFCodeGenerator::MallocPlaceholderType(llvm::IRBuilder<> builder) {
  auto placeholderIndex = builder.CreateAlloca(
        llvm::IntegerType::getInt32Ty(_module->getContext()),
        nullptr,
        "placeholder_index");
  CreateInputIntToCall(builder, placeholderIndex);
  auto placeholderIndexValue = builder.CreateLoad(placeholderIndex);
  CreatePrintfCall(builder, "placeholder_index= %d\n", placeholderIndexValue);

  auto placeholder = CreateGetFromObjectListCall(builder, placeholderIndexValue);
  return placeholder;
}

llvm::Value* CAFCodeGenerator::MallocFunctionType(llvm::IRBuilder<> builder) {
  auto cafCreateFunctionFunc = llvm::cast<llvm::Function>(
    _module->getOrInsertFunction(
      "_ZN9caf_v8lib18caf_CreateFunctionEPa", // caf_CreateFunction
      builder.getInt8PtrTy(),
      builder.getInt8PtrTy()
    )
  );

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
  auto func = builder.CreateIntToPtr(funcAsInt64, builder.getInt8PtrTy());

  auto functionValue = builder.CreateCall(cafCreateFunctionFunc, {func} );
  return functionValue;
}

llvm::Value* CAFCodeGenerator::MallocArrayType(llvm::IRBuilder<> builder) {
  auto inputArraySize = builder.CreateAlloca(
    llvm::IntegerType::getInt32Ty(_module->getContext()),
    nullptr,
    "input_arraysize");
  CreateInputIntToCall(builder, inputArraySize);
  auto inputArraySizeValue = dynamic_cast<llvm::Value *>(builder.CreateLoad(inputArraySize));
  CreatePrintfCall(builder, "input integer = %d\n", inputArraySizeValue); 
  auto arrayElements = builder.CreateAlloca(
    llvm::PointerType::getUnqual(builder.getInt8PtrTy()), inputArraySizeValue, "array_elements");

  auto whileCond = llvm::BasicBlock::Create(
      builder.getContext(), "malloc.array.while.cond",
      builder.GetInsertBlock()->getParent());
  auto whileBody = llvm::BasicBlock::Create(
      builder.getContext(), "malloc.array.while.body", 
      builder.GetInsertBlock()->getParent());
  auto mallocArrayEnd = llvm::BasicBlock::Create(
      builder.getContext(), "malloc.array.end", 
      builder.GetInsertBlock()->getParent());
  
  auto curIndex = builder.CreateAlloca(builder.getInt32Ty(), nullptr, "cur_array_index");
  builder.CreateStore(builder.getInt32(0), curIndex);

  auto whileBr = builder.CreateBr(whileCond);
  builder.SetInsertPoint(whileCond);
  {
    llvm::Value* curIndexValue = builder.CreateLoad(curIndex);
    llvm::Value* newArrayElementCond = builder.CreateICmpSLT(curIndexValue, inputArraySizeValue);
    // llvm::Value* scanfCond = builder.CreateICmpNE(scanfRes, zero);
    builder.CreateCondBr(newArrayElementCond, whileBody, mallocArrayEnd);
  }
  builder.SetInsertPoint(whileBody);
  {
    llvm::Value* curIndexValue = builder.CreateLoad(curIndex);


    auto mallocValueOfTypeFunc = llvm::cast<llvm::Function>(
      _module->getOrInsertFunction(
        "caf_dispatch_malloc_value_of_type",
        builder.getInt8PtrTy() // return a ptr of the malloced value of type
      )
    );
    auto curElement = builder.CreateCall(mallocValueOfTypeFunc);
    auto curElementToInt64Ptr = builder.CreateBitCast(
      curElement, 
      llvm::PointerType::getUnqual(builder.getInt8PtrTy())
    );
    llvm::Value *GEPIndices[] = { builder.getInt32(0), curIndexValue };
    llvm::Value * curAddr = builder.CreateInBoundsGEP(arrayElements, GEPIndices);
    builder.CreateStore(curElementToInt64Ptr, curAddr);

    // curIndex ++ 
    auto curIndexValueAddOne = builder.CreateAdd(curIndexValue, builder.getInt32(1));
    builder.CreateStore(curIndexValueAddOne, curIndex);
  }

  builder.SetInsertPoint(mallocArrayEnd);
  {
    auto cafCreateArrayFunc = llvm::cast<llvm::Function>(
      _module->getOrInsertFunction(
        "_ZN9caf_v8lib15caf_CreateArrayEPPai", // caf_CreateArray
        builder.getInt8PtrTy(),
        llvm::PointerType::getUnqual(builder.getInt8PtrTy()),
        builder.getInt32Ty()
      )
    );
    auto arrayValue = builder.CreateCall(cafCreateArrayFunc, { arrayElements, inputArraySizeValue });
    builder.CreateRet(arrayValue);
  }
}

llvm::Function* CAFCodeGenerator::CreateDispatchMallocValueOfType() {
  llvm::IRBuilder<> builder { _module->getcontext() };
  auto dispatchFunc = llvm::cast<llvm::Function>(
    _module->getOrInsertFunction(
      "caf_dispatch_malloc_value_of_type",
      builder.getInt8PtrTy() // return a ptr of the malloced value of type
    )
  );

  auto dispatchFuncEntry = llvm::BasicBlock::Create(
      dispatchFunc->getContext(),
      "caf.malloc.entry",
      dispatchFunc);
  
  std::vector<std::pair<llvm::ConstantInt *, llvm::BasicBlock *>> cases { };
  
  auto mallocUndefined = llvm::BasicBlock::Create(
      dispatchFunc->getContext(),
      "caf.malloc.undefine",
      dispatchFunc);
  builder.SetInsertPoint(mallocUndefined);
  auto undefinedValue = MallocUndefinedType(builder);
  builder.CreateRet(undefinedValue);
  cases.push_back(
    std::pair<llvm::ConstantInt *, llvm::BasicBlock *>
    (builder.getInt32(0), mallocUndefined)
  );

  auto mallocNull = llvm::BasicBlock::Create(
      dispatchFunc->getContext(),
      "caf.malloc.null",
      dispatchFunc);
  builder.SetInsertPoint(mallocNull);
  auto nullValue = MallocNullType(builder);
  builder.CreateRet(nullValue);

  cases.push_back(
    std::pair<llvm::ConstantInt *, llvm::BasicBlock *>
    (builder.getInt32(1), mallocNull)
  );
  auto mallocBoolean = llvm::BasicBlock::Create(
      dispatchFunc->getContext(),
      "caf.malloc.boolean",
      dispatchFunc);
  builder.SetInsertPoint(mallocBoolean);
  auto booleanValue = MallocBooleanType(builder);
  builder.CreateRet(booleanValue);

  cases.push_back(
    std::pair<llvm::ConstantInt *, llvm::BasicBlock *>
    (builder.getInt32(2), mallocBoolean)
  );
  auto mallocString = llvm::BasicBlock::Create(
      dispatchFunc->getContext(),
      "caf.malloc.string",
      dispatchFunc);
  builder.SetInsertPoint(mallocString);
  auto stringValue = MallocStringType(builder);
  builder.CreateRet(stringValue);
  cases.push_back(
    std::pair<llvm::ConstantInt *, llvm::BasicBlock *>
    (builder.getInt32(3), mallocString)
  );

  auto mallocFunction = llvm::BasicBlock::Create(
      dispatchFunc->getContext(),
      "caf.malloc.function",
      dispatchFunc);
  builder.SetInsertPoint(mallocFunction);
  auto functionValue = MallocFunctionType(builder);
  builder.CreateRet(functionValue);
  cases.push_back(
    std::pair<llvm::ConstantInt *, llvm::BasicBlock *>
    (builder.getInt32(4), mallocFunction)
  );

  auto mallocInteger = llvm::BasicBlock::Create(
      dispatchFunc->getContext(),
      "caf.malloc.integer",
      dispatchFunc);
  builder.SetInsertPoint(mallocInteger);
  auto integerValue = MallocIntegerType(builder);
  builder.CreateRet(integerValue);
  cases.push_back(
    std::pair<llvm::ConstantInt *, llvm::BasicBlock *>
    (builder.getInt32(5), mallocInteger)
  );

  auto mallocFloatingPointer = llvm::BasicBlock::Create(
      dispatchFunc->getContext(),
      "caf.malloc.float",
      dispatchFunc);
  builder.SetInsertPoint(mallocFloatingPointer);
  auto floatingPointerValue = MallocFloatingPointerType(builder);
  builder.CreateRet(floatingPointerValue);
  cases.push_back(
    std::pair<llvm::ConstantInt *, llvm::BasicBlock *>
    (builder.getInt32(6), mallocFloatingPointer)
  );

  auto mallocArray = llvm::BasicBlock::Create(
      dispatchFunc->getContext(),
      "caf.malloc.array",
      dispatchFunc);
  builder.SetInsertPoint(mallocArray);
  auto arrayValue = MallocArrayType(builder);
  builder.CreateRet(arrayValue);
  cases.push_back(
    std::pair<llvm::ConstantInt *, llvm::BasicBlock *>
    (builder.getInt32(7), mallocArray)
  );

  auto mallocPlaceholder = llvm::BasicBlock::Create(
      dispatchFunc->getContext(),
      "caf.malloc.placeholder",
      dispatchFunc);
  builder.SetInsertPoint(mallocPlaceholder);
  auto placeholderValue = MallocPlaceholderType(builder);
  builder.CreateRet(placeholderValue);

  cases.push_back(
    std::pair<llvm::ConstantInt *, llvm::BasicBlock *>
    (builder.getInt32(8), mallocPlaceholder)
  );

  // auto dispatchEnd = llvm::BasicBlock::Create(
  //     dispatchFunc->getContext(),
  //     "caf.malloc.end",
  //     dispatchFunc);

  builder.SetInsertPoint(dispatchFuncEntry);
  llvm::Value* inputKindValue;
  auto inputKind = builder.CreateAlloca(
      llvm::IntegerType::getInt32Ty(_module->getContext()),
      nullptr,
      "input_kind");
  CreateInputIntToCall(builder, inputKind);
  inputKindValue = builder.CreateLoad(inputKind);
  CreatePrintfCall(builder, "input_kind = %d\n", inputKindValue);

  auto mallocValueOfTypeSI = builder.CreateSwitch(
      inputKindValue, cases.front().second, cases.size());
  for (auto& c: cases) {
    mallocValueOfTypeSI->addCase(c.first, c.second);
  }

  return dispatchFunc;
}

void CAFCodeGenerator::GenerateCallbackFunctionCandidateArray() {
  llvm::IRBuilder<> builder { _module->getContext() };

  const auto& candidates = _extraction->GetApiFunctions();
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
  for(auto iter: candidates) {
    auto func = const_cast<llvm::Function *>(iter);
    auto apiId = _extraction->GetApiFunctionId(func).take();
    // func->dump();
    auto funcToStore = builder.CreatePtrToInt(func, builder.getInt64Ty());
    llvm::Value* arrIndex[] = { builder.getInt32(0), builder.getInt32(apiId) };
    auto curAddrToStore = builder.CreateGEP(callbackFuncArray, arrIndex );
    builder.CreateStore(funcToStore, curAddrToStore);
  }

  builder.CreateBr(EndBlock);
  builder.SetInsertPoint(EndBlock);
  builder.CreateRetVoid();

  // add attribute: optnone unwind
  {
    llvm::AttrBuilder B;
    B.addAttribute(llvm::Attribute::NoInline);
    B.addAttribute(llvm::Attribute::OptimizeNone);
    B.addAttribute(llvm::Attribute::UWTable);
    callbackFuncArrDispatch->addAttributes(llvm::AttributeList::FunctionIndex, B);
  }
}

// llvm::Value* CAFCodeGenerator::mallocValueOfType(
//     llvm::IRBuilder<>& builder, llvm::Type* type) {

//   llvm::Value* inputKindValue;
//   auto inputKind = builder.CreateAlloca(
//       llvm::IntegerType::getInt32Ty(_module->getContext()),
//       nullptr,
//       "input_kind");
//   CreateInputIntToCall(builder, inputKind);
//   inputKindValue = builder.CreateLoad(inputKind);
//   CreatePrintfCall(builder, "input_kind = %d\n", inputKindValue);

// auto isPlaceHolder = builder.CreateICmpEQ(inputKindValue, builder.getInt32(6), "is_place_holder");
//   auto thenBlock = llvm::BasicBlock::Create(
//       builder.getContext(), "reuse.object.then",
//       builder.GetInsertBlock()->getParent());
//   auto elseBlock = llvm::BasicBlock::Create(
//       builder.getContext(), "reuse.object.else",
//       builder.GetInsertBlock()->getParent());
//   auto afterBlock = llvm::BasicBlock::Create(
//       builder.getContext(),"after.malloc.object",
//       builder.GetInsertBlock()->getParent());

//   builder.CreateCondBr(isPlaceHolder, thenBlock, elseBlock);

//   builder.SetInsertPoint(afterBlock);
//   auto phiObj = builder.CreatePHI(type->getPointerTo(), 2);

//   builder.SetInsertPoint(thenBlock);
//   // {
//     auto objectIdx = builder.CreateAlloca(
//           llvm::IntegerType::getInt32Ty(_module->getContext()),
//           nullptr,
//           "object_idx");
//     CreateInputIntToCall(builder, objectIdx);
//     auto objectIdxValue = builder.CreateLoad(objectIdx);
//     CreatePrintfCall(builder, "object_idx = %d\n", objectIdxValue);

//     auto ObjPtrToInt64 = CreateGetFromObjectListCall(builder, objectIdxValue);
//     auto thenRet = builder.CreateIntToPtr(ObjPtrToInt64, type->getPointerTo());
//     builder.CreateBr(afterBlock);
//     phiObj->addIncoming(thenRet, builder.GetInsertBlock());
//   // }

//   llvm::Value* elseRet;
//   builder.SetInsertPoint(elseBlock);
//   // {
//     if (type->isIntegerTy() || type->isFloatingPointTy()) { // input_kind == 0
//       llvm::Value* valueAddr = CreateMallocCall(builder, type);
//       if(init) {
//         CreatePrintfCall(builder, "bitsType: ");
//         CreateSaveToObjectListCall(builder, builder.CreatePtrToInt(valueAddr, builder.getInt64Ty()));
//       }
  
//       // insertBefore->eraseFromParent();
//       if(init == true && depth < CAF_RECURSIVE_MAX_DEPTH) { // input_kind = 0
//         auto inputSize = builder.CreateAlloca(
//           llvm::IntegerType::getInt32Ty(_module->getContext()),
//           nullptr,
//           "input_size");
//         CreateInputIntToCall(builder, inputSize);
//         auto inputSizeValue = dynamic_cast<llvm::Value *>(builder.CreateLoad(inputSize));
//         CreatePrintfCall(builder, "input size = %d\n", inputSizeValue);

//         CreateInputBtyesToCall(builder, valueAddr, inputSizeValue);
//         CreatePrintfCall(builder, "input value = %d\n", builder.CreateLoad(valueAddr));
//       }
//       elseRet = valueAddr;
//     } else if (type->isStructTy()) {  // input_kind = 1
//       elseRet = AllocaStructValue(builder, llvm::dyn_cast<llvm::StructType>(type), depth, init);
//     } else if (type->isPointerTy()) { // input_kind = 2
//       elseRet = AllocaPointerType(builder, llvm::dyn_cast<llvm::PointerType>(type), depth, init);
//     } else if (type->isArrayTy()) {  // input_kind = 3
//       elseRet = AllocaArrayType(builder, llvm::dyn_cast<llvm::ArrayType>(type), depth, init);
//     } else if(type->isVectorTy()) {  // input_kind = 3
//       elseRet = AllocaVectorType(builder, llvm::dyn_cast<llvm::VectorType>(type), depth, init);
//     } else if (type->isFunctionTy()) {  // input_kind = 5
//       elseRet = AllocaFunctionType(builder, llvm::dyn_cast<llvm::FunctionType>(type), depth, init);
//     } else {
//       llvm::errs() << "Trying to alloca unrecognized type: "
//           << type->getTypeID() << "\n";
//       // type->dump(); 
//       elseRet = CreateMallocCall(builder, type);
//       if(init && depth < CAF_RECURSIVE_MAX_DEPTH) {
//         CreatePrintfCall(builder, "unknownType: ");
//         CreateSaveToObjectListCall(builder, builder.CreatePtrToInt(elseRet, builder.getInt64Ty()));
//       }
  
//       elseRet = builder.CreateAlloca(type);
//     }

//     // auto typeId = _extraction->GetTypeId(type).take();
//     // auto mallocForTypeFunc = llvm::cast<llvm::Function>(
//     //   _module->getOrInsertFunction(
//     //       "__caf_dispatch_malloc_for_type_" + std::to_string(typeId),
//     //       llvm::Type::getInt8PtrTy(_module->getContext()),
//     //       llvm::Type::getInt8Ty(_module->getContext()) 
//     //   )
//     // );
//     // CreatePrintfCall(builder, "__caf_dispatch_malloc_for_type_" + std::to_string(typeId));
//     // auto malloc = builder.CreateCall(mallocForTypeFunc, {builder.getInt8(init)} );

//     // auto mallocOfType = builder.CreateBitCast(malloc, type->getPointerTo());
//     // elseRet = mallocOfType;

//     // builder.SetInsertPoint(elseBlock);
//     builder.CreateBr(afterBlock);
//     phiObj->addIncoming(elseRet, builder.GetInsertBlock());
//   // }
//   builder.SetInsertPoint(afterBlock);

//   // CreateSaveToObjectListCall(builder, builder.CreatePtrToInt(phiObj, builder.getInt64Ty()));
//   return phiObj;
// }

std::pair<llvm::ConstantInt *, llvm::BasicBlock *> CAFCodeGenerator::CreateCallApiCase(
    llvm::Function* callee, llvm::Function* caller, int caseCounter) {
  llvm::IRBuilder<> builder { caller->getContext() };
  auto module = caller->getParent();
  auto& context = module->getContext();

  std::string block_name("invoke.case.");
  block_name.append(std::to_string(caseCounter));
  auto invokeApiCase = llvm::BasicBlock::Create(
      caller->getContext(), block_name, caller);

  builder.SetInsertPoint(invokeApiCase);
  std::string calleeName = demangle(callee->getName().str());
  CreatePrintfCall(builder, calleeName);

  std::vector<llvm::Value *> calleeArgs { };
  
  // testcase: <this: Value> <argsCount: u32> <args: [argsCount x Value]>
  auto mallocValueOfTypeFunc = llvm::cast<llvm::Function>(
    _module->getOrInsertFunction(
      "caf_dispatch_malloc_value_of_type",
      builder.getInt8PtrTy() // return a ptr of the malloced value of type
    )
  );
  auto thisArg = builder.CreateBitCast(builder.CreateCall(mallocValueOfTypeFunc),
    builder.getInt8PtrTy());

  auto argsCount = builder.CreateAlloca(
      llvm::IntegerType::getInt32Ty(_module->getContext()),
      nullptr,
      "args_count");
  CreateInputIntToCall(builder, argsCount);
  auto argsCountValue = builder.CreateLoad(argsCount);
  CreatePrintfCall(builder, "args_count = %d\n", argsCountValue);

  // Address caf_CreateFunctionCallbackInfo(Address* implicit_args, Address* values, int length);
  auto implicit_args = builder.CreateAlloca(
    builder.getInt8PtrTy(),
    builder.getInt32(6),
    "implicit_args"
  );
  auto values = builder.CreateAlloca(
    builder.getInt8PtrTy(),
    argsCountValue,
    "values"
  );

  // store <this: Value> at implicit_args[0]
  llvm::Value *GEPIndices[] = { builder.getInt32(0), builder.getInt32(0) }; 
  llvm::Value * curAddr = builder.CreateInBoundsGEP(implicit_args, GEPIndices);
  builder.CreateStore(thisArg, curAddr);

  auto whileCond = llvm::BasicBlock::Create(
      builder.getContext(), "malloc.args.while.cond",
      builder.GetInsertBlock()->getParent());
  auto whileBody = llvm::BasicBlock::Create(
      builder.getContext(), "malloc.args.while.body", 
      builder.GetInsertBlock()->getParent());
  auto mallocArgsEnd = llvm::BasicBlock::Create(
      builder.getContext(), "malloc.args.end", 
      builder.GetInsertBlock()->getParent());
  
  auto curArgsNum = builder.CreateAlloca(builder.getInt32Ty(), nullptr, "cur_args_num");
  builder.CreateStore(builder.getInt32(0), curArgsNum);

  auto whileBr = builder.CreateBr(whileCond);
  builder.SetInsertPoint(whileCond);
  {
    llvm::Value* curArgsNumValue = builder.CreateLoad(curArgsNum);
    llvm::Value* newArgsCond = builder.CreateICmpSLT(curArgsNumValue, argsCountValue);
    // llvm::Value* scanfCond = builder.CreateICmpNE(scanfRes, zero);
    builder.CreateCondBr(newArgsCond, whileBody, mallocArgsEnd);
  }
  builder.SetInsertPoint(whileBody);
  {
    llvm::Value* curArgsNumValue = builder.CreateLoad(curArgsNum);

    auto curArgs = builder.CreateCall(mallocValueOfTypeFunc);
    auto curArgsToInt64Ptr = builder.CreateBitCast(
      curArgs, 
      builder.getInt8PtrTy()
    );
    llvm::Value *GEPIndices[] = { builder.getInt32(0), curArgsNumValue };
    llvm::Value * curAddr = builder.CreateInBoundsGEP(values, GEPIndices);
    builder.CreateStore(curArgsToInt64Ptr, curAddr);

    // curIndex ++ 
    auto curIndexValueAddOne = builder.CreateAdd(curArgsNumValue, builder.getInt32(1));
    builder.CreateStore(curIndexValueAddOne, curArgsNum);
  }

  builder.SetInsertPoint(mallocArgsEnd);
  {
    llvm::Type* functionCallbackInfoType;
    for(auto &arg: callee->args()) {
      functionCallbackInfoType = arg.getType();
    }

    functionCallbackInfoType->dump();
    auto mallocFunctionCallbackInfo = llvm::cast<llvm::Function>(
      _module->getOrInsertFunction(
        "_ZN9caf_v8lib30caf_CreateFunctionCallbackInfoEPPaS1_i", // caf_CreateFunctionCallbackInfo
        functionCallbackInfoType, // return a ptr of the malloced value of type
        builder.getInt8PtrTy(),
        builder.getInt8PtrTy(),
        builder.getInt32Ty()
      )
    );
    mallocFunctionCallbackInfo->dump();
    llvm::Value* functionCallbackInfoValue = builder.CreateCall(
      mallocFunctionCallbackInfo, 
      { implicit_args, values, argsCountValue }
    );


    // CreatePrintfCall(builder, "args created.\n");
    builder.CreateCall(callee, { functionCallbackInfoValue } );
    auto getReturnValueFunc = llvm::cast<llvm::Function>(
      _module->getOrInsertFunction(
        "_ZN9caf_v8lib15caf_GetRetValueEN2v820FunctionCallbackInfoINS0_5ValueEEE", // caf_GetRetValue
        builder.getInt8PtrTy(), // return a ptr of the malloced value of type
        functionCallbackInfoType
      )
    );
    auto returnValue = builder.CreateCall(getReturnValueFunc, { functionCallbackInfoValue });
    CreatePrintfCall(builder, "retType:\tsave to object list.\n");
    CreateSaveToObjectListCall(builder, builder.CreatePtrToInt(returnValue, builder.getInt64Ty()));
  }

  auto caseId = llvm::ConstantInt::get(
      llvm::Type::getInt32Ty(context), caseCounter);
  return std::make_pair(caseId, invokeApiCase);
}

// static std::string getSignature(llvm::FunctionType *FTy) {
//   std::string Sig;
//   llvm::raw_string_ostream OS(Sig);
//   OS << *FTy->getReturnType();
//   for (llvm::Type *ParamTy : FTy->params())
//     OS << "_" << *ParamTy;
//   if (FTy->isVarArg())
//     OS << "_...";
//   Sig = OS.str();
//   Sig.erase(llvm::remove_if(Sig, isspace), Sig.end());
//   // When s2wasm parses .s file, a comma means the end of an argument. So a
//   // mangled function name can contain any character but a comma.
//   std::replace(Sig.begin(), Sig.end(), ',', '.');
//   return Sig;
// }

} // namespace caf
