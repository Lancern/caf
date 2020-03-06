#include "Extractor/ExtractorContext.h"
#include "CAFCodeGenerator.h"

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
  // auto ctors = _extraction->GetConstructors();
  auto types = _extraction->GetTypes();

  llvm::errs() << "typeIds.size(): " << types.size() << "\n";
  for(auto type: types) {
    if(!llvm::isa<llvm::StructType>(type)) continue;
    auto ctors = _extraction->GetConstructorsOfType(type);
    if(ctors.size() == 0)continue;
    auto typeId = _extraction->GetTypeId(type).take();
    CreateDispatchFunctionForCtors(typeId);
  }

  for(auto type: types) {
    if(type->isVoidTy())continue;
    CreateDispatchFunctionForTypes(const_cast<llvm::Type*>(type));
  }


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

llvm::Function* CAFCodeGenerator::CreateDispatchFunctionForTypes(llvm::Type* type) {
  auto typeId = _extraction->GetTypeId(type).take();
  auto dispatchFunc = llvm::cast<llvm::Function>(
    _module->getOrInsertFunction(
        "__caf_dispatch_malloc_for_type_" + std::to_string(typeId),
        llvm::Type::getInt8PtrTy(_module->getContext())
        // ,llvm::Type::getInt8Ty(_module->getContext()) 
        // depth: must <= CAF_RECURSIVE_MAX_DEPTH. Otherwise, do not initialize
    )
  );
  dispatchFunc->setCallingConv(llvm::CallingConv::C);

  llvm::IRBuilder<> builder { dispatchFunc->getContext() };

  auto mallocTypeEntry = llvm::BasicBlock::Create(
      dispatchFunc->getContext(),
       + "malloc.type." + std::to_string(typeId) + ".entry",
      dispatchFunc);
  auto mallocTypeEnd = llvm::BasicBlock::Create(
      dispatchFunc->getContext(),
       + "malloc.type." + std::to_string(typeId) + ".end",
      dispatchFunc);
    
  builder.SetInsertPoint(mallocTypeEntry);

  llvm::Value* isPlaceHolderAddr = builder.CreateAlloca(
        builder.getInt8Ty(),
        nullptr,
        "is_placeholder.addr");
    CreateInputBtyesToCall(builder, isPlaceHolderAddr, builder.getInt32(1));
    auto isPlaceHolder = builder.CreateLoad(isPlaceHolderAddr);
    CreatePrintfCall(builder, "is_placeholder = %d\n", isPlaceHolder);

  llvm::Value* mallocOfType;
  llvm::Value* mallocOfPointerType;
  if(type->isFunctionTy()) {
    auto pointerType = type->getPointerTo();
    mallocOfPointerType = CreateMallocCall(builder, pointerType);
    mallocOfType = builder.CreateLoad(mallocOfPointerType);
  }else {
    mallocOfType = CreateMallocCall(builder, type);
  }
  type->dump();

  auto placeHolderBlock = llvm::BasicBlock::Create(
      builder.getContext(), "placeholder.type." + std::to_string(typeId),
      builder.GetInsertBlock()->getParent());
  auto noPlaceHolderBlock = llvm::BasicBlock::Create(
      builder.getContext(), "no.placeholder.type." + std::to_string(typeId),
      builder.GetInsertBlock()->getParent());

  auto hasPlaceHolder = builder.CreateICmpEQ(isPlaceHolder, builder.getInt8(1), "init_or_not");
  builder.CreateCondBr(hasPlaceHolder, placeHolderBlock, noPlaceHolderBlock);

  builder.SetInsertPoint(mallocTypeEnd);
  auto phiRet = builder.CreatePHI(type->getPointerTo(), 2);

  // no initialize the type.
  builder.SetInsertPoint(placeHolderBlock);
  // TODO: initialize the type with null.
  auto objectIdx = builder.CreateAlloca(
        llvm::IntegerType::getInt32Ty(_module->getContext()),
        nullptr,
        "object_index");
  CreateInputIntToCall(builder, objectIdx);
  auto objectIdxValue = builder.CreateLoad(objectIdx);
  CreatePrintfCall(builder, "object_index = %d\n", objectIdxValue);

  auto objPtrToInt64 = CreateGetFromObjectListCall(builder, objectIdxValue);
  auto objPtr = builder.CreateIntToPtr(objPtrToInt64, type->getPointerTo());
  // builder.CreateStore(builder.CreateLoad(objPtr), mallocOfType);
  builder.CreateBr(mallocTypeEnd);
  phiRet->addIncoming(objPtr, builder.GetInsertBlock());

  // initialize the type.
  builder.SetInsertPoint(noPlaceHolderBlock);
  CreateSaveToObjectListCall(builder, builder.CreatePtrToInt(mallocOfType, builder.getInt64Ty()));
  if(type->isIntegerTy() || type->isFloatingPointTy()) { // kind = 0, bitsValue
      CreatePrintfCall(builder, "bitsType:\tsave to object list.\n");
      auto inputSize = builder.CreateAlloca(
        llvm::IntegerType::getInt32Ty(_module->getContext()),
        nullptr,
        "input_size");
      CreateInputIntToCall(builder, inputSize);
      auto inputSizeValue = dynamic_cast<llvm::Value *>(builder.CreateLoad(inputSize));
      CreatePrintfCall(builder, "input size = %d\n", inputSizeValue);

      CreateInputBtyesToCall(builder, mallocOfType, inputSizeValue);
      CreatePrintfCall(builder, "input value = %d\n", builder.CreateLoad(mallocOfType));
  } else if(type->isPointerTy()) { // kind = 1, pointerValue
    CreatePrintfCall(builder, "pointerType:\tsave to object list.\n");
    llvm::Type* pointeeType = type->getPointerElementType();
    auto pointeeTypeId = _extraction->GetTypeId(pointeeType).take();
    auto mallocForPointeeTypeFunc = llvm::cast<llvm::Function>(
      _module->getOrInsertFunction(
          "__caf_dispatch_malloc_for_type_" + std::to_string(pointeeTypeId),
          llvm::Type::getInt8PtrTy(_module->getContext())
          //, llvm::Type::getInt8Ty(_module->getContext()) 
      )
    );
    auto pointeeOfInt8ptr = builder.CreateCall(mallocForPointeeTypeFunc);
    auto pointeeValue = builder.CreateBitCast(pointeeOfInt8ptr, pointeeType->getPointerTo());
    builder.CreateStore(pointeeValue, mallocOfType);
  } else if(type->isFunctionTy()) {
    CreatePrintfCall(builder, "functionType:\tsave to object list.\n");
    auto pointerType = type->getPointerTo();
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
    builder.CreateStore(func, mallocOfPointerType);
  } else if(type->isArrayTy() || type->isVectorTy()) {
    CreatePrintfCall(builder, "arrayOrVectorType:\tsave to object list.\n");
    // auto arraySize = builder.CreateAlloca(
    //   llvm::IntegerType::getInt32Ty(_module->getContext()),
    //   nullptr,
    //   "array_size");
    // CreateInputIntToCall(builder, arraySize);
    // auto arraySizeValue = dynamic_cast<llvm::Value *>(builder.CreateLoad(arraySize));
    // CreatePrintfCall(builder, "array size = %d\n", arraySizeValue);

    uint64_t arrsize = type->getArrayNumElements();
    llvm::Type * elementType = type->getArrayElementType();
    auto elementTypeId = _extraction->GetTypeId(elementType).take();
    auto mallocForElementTypeFunc = llvm::cast<llvm::Function>(
      _module->getOrInsertFunction(
          "__caf_dispatch_malloc_for_type_" + std::to_string(elementTypeId),
          llvm::Type::getInt8PtrTy(_module->getContext())
          //, llvm::Type::getInt8Ty(_module->getContext()) 
      )
    );
    // uint64_t elementSize = elementType->getScalarSizeInBits();
    for(uint64_t i = 0; i < arrsize; i++)
    {
      llvm::Value* elementOfInt8Ptr = builder.CreateCall(mallocForElementTypeFunc);
      // llvm::Value* elementOfInt8Ptr = AllocaValueOfType(builder, elementType);
      auto element = builder.CreateBitCast(elementOfInt8Ptr, elementType->getPointerTo());
      llvm::Value* elementValue = builder.CreateLoad(element);
      llvm::Value *GEPIndices[] = { builder.getInt32(0), builder.getInt32(i) };
      llvm::Value * curAddr = builder.CreateInBoundsGEP(mallocOfType, GEPIndices);
      builder.CreateStore(elementValue, curAddr);
    }
  } else if(type->isStructTy()) {
    llvm::StructType* stype = llvm::cast<llvm::StructType>(type);
    std::string tname("");
    if(!stype->isLiteral()) {
      tname = stype->getStructName().str();
      auto found = tname.find("."); //class.basename / struct.basename
      std::string basename = tname.substr(found + 1);
    }
    auto dispatchFunc = _module->getFunction(
      "__caf_dispatch_ctor_" + tname
    );
    if(dispatchFunc) {
      CreatePrintfCall(builder, "structType:\tsave to object list.\n");
      auto ctorId = builder.CreateAlloca(
        llvm::IntegerType::getInt32Ty(_module->getContext()),
        nullptr,
        "ctor_id");
      CreateInputIntToCall(builder, ctorId);
      auto ctorIdValue = dynamic_cast<llvm::Value *>(builder.CreateLoad(ctorId));
      CreatePrintfCall(builder, "ctor_id = %d\n", ctorIdValue);

      llvm::Value* params[] = { ctorIdValue };
      auto ctorSret = builder.CreateCall(dispatchFunc, params);
      unsigned TypeSize = _module->getDataLayout().getTypeAllocSize(type);
      CreateMemcpyCall(builder, mallocOfType, ctorSret, llvm::ConstantInt::get(builder.getInt64Ty(), TypeSize));
    } else {
      CreatePrintfCall(builder, "aggregateType:\tsave to object list.\n");
      int elementId = 0;
      for(auto elementType: stype->elements()) {
        auto elementTypeId = _extraction->GetTypeId(elementType).take();
        auto mallocForElementTypeFunc = llvm::cast<llvm::Function>(
          _module->getOrInsertFunction(
              "__caf_dispatch_malloc_for_type_" + std::to_string(elementTypeId),
              llvm::Type::getInt8PtrTy(_module->getContext())
              //, llvm::Type::getInt8Ty(_module->getContext()) 
          )
        );
        llvm::Value* elementOfInt8Ptr = builder.CreateCall(mallocForElementTypeFunc);
        // llvm::Value* elementOfInt8Ptr = AllocaValueOfType(builder, elementType);
        auto element = builder.CreateBitCast(elementOfInt8Ptr, elementType->getPointerTo());
        llvm::Value* elementValue = builder.CreateLoad(element);
        llvm::Value *GEPIndices[] = { builder.getInt32(0), builder.getInt32(elementId++) };
        llvm::Value * curAddr = builder.CreateInBoundsGEP(mallocOfType, GEPIndices);
        builder.CreateStore(elementValue, curAddr);
      }
    }
  }else {

  }
  builder.CreateBr(mallocTypeEnd);
  phiRet->addIncoming(mallocOfType, builder.GetInsertBlock());

  builder.SetInsertPoint(mallocTypeEnd);
  auto mallocOfInt8Ptr = builder.CreateBitCast(phiRet, builder.getInt8PtrTy());
  builder.CreateRet(mallocOfInt8Ptr);
  // add attribute: optnone unwind
  {
    llvm::AttrBuilder B;
    B.addAttribute(llvm::Attribute::NoInline);
    B.addAttribute(llvm::Attribute::OptimizeNone);
    B.addAttribute(llvm::Attribute::UWTable);
    dispatchFunc->addAttributes(llvm::AttributeList::FunctionIndex, B);
  }

  // dispatchFunc->dump();

  return dispatchFunc;
}


llvm::Function* CAFCodeGenerator::CreateDispatchFunctionForCtors(uint64_t ctorTypeId) {
  auto ctorType = _extraction->GetTypeById(ctorTypeId).take();
  auto ctors = _extraction->GetConstructorsOfType(ctorType);

  std::string dispatchType = std::string("ctor");
  llvm::Function* dispatchFunc;
  auto type = _extraction->GetTypeById(ctorTypeId).take();

  std::string tname = type->getStructName().str();
  auto found = tname.find("."); //class.basename / struct.basename
  std::string basename = tname.substr(found + 1);

  // llvm::errs() << tname + " ctor dispatch function creating ...\n";

  dispatchFunc = llvm::cast<llvm::Function>(
    _module->getOrInsertFunction(
        "__caf_dispatch_ctor_" + tname,
        llvm::Type::getInt8PtrTy(_module->getContext()),
        llvm::Type::getInt32Ty(_module->getContext())
    )
  );
  dispatchFunc->setCallingConv(llvm::CallingConv::C);

  auto argApiId = dispatchFunc->arg_begin();
  argApiId->setName(dispatchType);

  llvm::IRBuilder<> builder { dispatchFunc->getContext() };
  auto invokeApiEntry = llvm::BasicBlock::Create(
      dispatchFunc->getContext(),
      "caf.ctor." + tname + ".entry",
      dispatchFunc);

  std::vector<std::pair<llvm::ConstantInt *, llvm::BasicBlock *>> cases { };

  // llvm::errs() << " *** *** *** *** *** *** \n";
  // type->dump();
  // llvm::errs() << "ctorTypeId = " << ctorTypeId << " - ctors.size() = " << ctors.size() << "\n";
  // llvm::errs() << " *** *** *** *** *** *** \n";
  // int caseCounter = 0;
  llvm::errs() << tname + " ctor dispatch function creating ...\n";
  for (auto ctor: ctors) {
    auto ctorId =  _extraction->GetConstructorId(ctor).take();
    // llvm::errs() << "generate case for ctorId: " << ctorId << "\n";
    cases.push_back(CreateCallApiCase(
        const_cast<llvm::Function *>(ctor), dispatchFunc, ctorId, true));
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
        "caf.ctor." + tname + ".defualt",
        dispatchFunc);
    auto invokeApiEnd = llvm::BasicBlock::Create(
        dispatchFunc->getContext(),
        "caf.ctor." + tname + ".end",
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
  llvm::errs() << "ctor dispatchFunc created successfully.\n";
  return dispatchFunc;
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
    cases.push_back(CreateCallApiCase(
        const_cast<llvm::Function *>(func), dispatchFunc, caseCounter++));
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

llvm::Value* CAFCodeGenerator::AllocaStructValue(
    llvm::IRBuilder<>& builder, llvm::StructType* type, int depth, bool init) {
  // init = false;
  // llvm::Value* argAlloca = builder.CreateAlloca(type);
  llvm::Value* argAlloca = CreateMallocCall(builder, type);
  if(init) {
    CreatePrintfCall(builder, "structType: ");
    CreateSaveToObjectListCall(builder, builder.CreatePtrToInt(argAlloca, builder.getInt64Ty()));
  }

  if(init == false || depth >= CAF_RECURSIVE_MAX_DEPTH)return argAlloca;

  auto ctorId = builder.CreateAlloca(
    llvm::IntegerType::getInt32Ty(_module->getContext()),
    nullptr,
    "ctor_id");
  CreateInputIntToCall(builder, ctorId);
  auto ctorIdValue = dynamic_cast<llvm::Value *>(builder.CreateLoad(ctorId));
  CreatePrintfCall(builder, "ctor_id = %d\n", ctorIdValue);

  auto stype = llvm::dyn_cast<llvm::StructType>(type);

  std::string tname("");
  if(!type->isLiteral()) {
    tname = type->getStructName().str();
    auto found = tname.find("."); //class.basename / struct.basename
    std::string basename = tname.substr(found + 1);
  }
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
    // llvm::errs() << tname << " has no ctors.\n\n";
    int elementId = 0;
    for(auto elementType: type->elements()) {
      // llvm::errs() << tname << " elementId: " << elementId << "\n";
      llvm::Value* element = AllocaValueOfType(builder, elementType, depth + 1, init);
      llvm::Value* elementValue = builder.CreateLoad(element);
      llvm::Value *GEPIndices[] = { builder.getInt32(0), builder.getInt32(elementId++) };
      llvm::Value * curAddr = builder.CreateInBoundsGEP(argAlloca, GEPIndices);
      builder.CreateStore(elementValue, curAddr);
    }
    // llvm::errs() << tname << " type created done.\n";
  }

  return argAlloca;
}

llvm::Value* CAFCodeGenerator::AllocaPointerType(
    llvm::IRBuilder<>& builder, llvm::PointerType* type, int depth, bool init) {

  llvm::Value* argAlloca = CreateMallocCall(builder, type);
  
  if(init) {
    CreatePrintfCall(builder, "pointerType: ");
    CreateSaveToObjectListCall(builder, builder.CreatePtrToInt(argAlloca, builder.getInt64Ty()));
  }
  
  // auto argAlloca = builder.CreateAlloca(type); // 按类型分配地址空间
  // if(init == false)return argAlloca;

  llvm::Type* pointeeType = type->getPointerElementType();

  if (depth >= CAF_RECURSIVE_MAX_DEPTH || !CanAlloca(pointeeType) || init == false) {
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
  if(init) {
    CreatePrintfCall(builder, "arrayType: ");
    CreateSaveToObjectListCall(builder, builder.CreatePtrToInt(arrayAddr, builder.getInt64Ty()));
  }
  
  // auto arrayAddr = builder.CreateAlloca(type);

  if(init == true && depth < CAF_RECURSIVE_MAX_DEPTH) {
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
      llvm::Value* element = AllocaValueOfType(builder, elementType, depth + 1, init);
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
  if(init) {
    CreatePrintfCall(builder, "vectorType: ");
    CreateSaveToObjectListCall(builder, builder.CreatePtrToInt(vectorAddr, builder.getInt64Ty()));
  }
  
  // auto vectorAddr = builder.CreateAlloca(type);

  if(init == true && depth < CAF_RECURSIVE_MAX_DEPTH) {
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
      llvm::Value* element = AllocaValueOfType(builder, elementType, depth + 1, init);
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
  if(init == true && depth < CAF_RECURSIVE_MAX_DEPTH) {
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
  if(init) {
    CreatePrintfCall(builder, "functionType: ");
    CreateSaveToObjectListCall(builder, builder.CreatePtrToInt(argAlloca, builder.getInt64Ty()));
  }
  
  return argAlloca;
}

llvm::Value* CAFCodeGenerator::AllocaValueOfType(
    llvm::IRBuilder<>& builder, llvm::Type* type, int depth, bool init) {

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
      inputKindValue = builder.getInt32(3);
    } else if(type->isStructTy()) {
      inputKindValue = builder.getInt32(4);
    } else if(type->isFunctionTy()) {
      inputKindValue = builder.getInt32(2);
    }
  }

  
auto isPlaceHolder = builder.CreateICmpEQ(inputKindValue, builder.getInt32(6), "is_place_holder");
  auto thenBlock = llvm::BasicBlock::Create(
      builder.getContext(), "reuse.object.then",
      builder.GetInsertBlock()->getParent());
  auto elseBlock = llvm::BasicBlock::Create(
      builder.getContext(), "reuse.object.else",
      builder.GetInsertBlock()->getParent());
  auto afterBlock = llvm::BasicBlock::Create(
      builder.getContext(),"after.malloc.object",
      builder.GetInsertBlock()->getParent());

  builder.CreateCondBr(isPlaceHolder, thenBlock, elseBlock);

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
    // if (type->isIntegerTy() || type->isFloatingPointTy()) { // input_kind == 0
    //   llvm::Value* valueAddr = CreateMallocCall(builder, type);
    //   if(init) {
    //     CreatePrintfCall(builder, "bitsType: ");
    //     CreateSaveToObjectListCall(builder, builder.CreatePtrToInt(valueAddr, builder.getInt64Ty()));
    //   }
  
    //   // insertBefore->eraseFromParent();
    //   if(init == true && depth < CAF_RECURSIVE_MAX_DEPTH) { // input_kind = 0
    //     auto inputSize = builder.CreateAlloca(
    //       llvm::IntegerType::getInt32Ty(_module->getContext()),
    //       nullptr,
    //       "input_size");
    //     CreateInputIntToCall(builder, inputSize);
    //     auto inputSizeValue = dynamic_cast<llvm::Value *>(builder.CreateLoad(inputSize));
    //     CreatePrintfCall(builder, "input size = %d\n", inputSizeValue);

    //     CreateInputBtyesToCall(builder, valueAddr, inputSizeValue);
    //     CreatePrintfCall(builder, "input value = %d\n", builder.CreateLoad(valueAddr));
    //   }
    //   elseRet = valueAddr;
    // } else if (type->isStructTy()) {  // input_kind = 1
    //   elseRet = AllocaStructValue(builder, llvm::dyn_cast<llvm::StructType>(type), depth, init);
    // } else if (type->isPointerTy()) { // input_kind = 2
    //   elseRet = AllocaPointerType(builder, llvm::dyn_cast<llvm::PointerType>(type), depth, init);
    // } else if (type->isArrayTy()) {  // input_kind = 3
    //   elseRet = AllocaArrayType(builder, llvm::dyn_cast<llvm::ArrayType>(type), depth, init);
    // } else if(type->isVectorTy()) {  // input_kind = 3
    //   elseRet = AllocaVectorType(builder, llvm::dyn_cast<llvm::VectorType>(type), depth, init);
    // } else if (type->isFunctionTy()) {  // input_kind = 5
    //   elseRet = AllocaFunctionType(builder, llvm::dyn_cast<llvm::FunctionType>(type), depth, init);
    // } else {
    //   llvm::errs() << "Trying to alloca unrecognized type: "
    //       << type->getTypeID() << "\n";
    //   // type->dump(); 
    //   elseRet = CreateMallocCall(builder, type);
    //   if(init && depth < CAF_RECURSIVE_MAX_DEPTH) {
    //     CreatePrintfCall(builder, "unknownType: ");
    //     CreateSaveToObjectListCall(builder, builder.CreatePtrToInt(elseRet, builder.getInt64Ty()));
    //   }
  
    //   // elseRet = builder.CreateAlloca(type);
    // }

    auto typeId = _extraction->GetTypeId(type).take();
    auto mallocForTypeFunc = llvm::cast<llvm::Function>(
      _module->getOrInsertFunction(
          "__caf_dispatch_malloc_for_type_" + std::to_string(typeId),
          llvm::Type::getInt8PtrTy(_module->getContext()),
          llvm::Type::getInt8Ty(_module->getContext()) 
      )
    );
    CreatePrintfCall(builder, "__caf_dispatch_malloc_for_type_" + std::to_string(typeId));
    auto malloc = builder.CreateCall(mallocForTypeFunc, {builder.getInt8(init)} );

    auto mallocOfType = builder.CreateBitCast(malloc, type->getPointerTo());
    elseRet = mallocOfType;

    // builder.SetInsertPoint(elseBlock);
    builder.CreateBr(afterBlock);
    phiObj->addIncoming(elseRet, builder.GetInsertBlock());
  // }
  builder.SetInsertPoint(afterBlock);

  // CreateSaveToObjectListCall(builder, builder.CreatePtrToInt(phiObj, builder.getInt64Ty()));
  return phiObj;
}

std::pair<llvm::ConstantInt *, llvm::BasicBlock *> CAFCodeGenerator::CreateCallApiCase(
    llvm::Function* callee, llvm::Function* caller, int caseCounter, bool createForCtor) {
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
    auto type = arg.getType();
    auto typeId = _extraction->GetTypeId(type).take();
    auto mallocForTypeFunc = llvm::cast<llvm::Function>(
      _module->getOrInsertFunction(
          "__caf_dispatch_malloc_for_type_" + std::to_string(typeId),
          llvm::Type::getInt8PtrTy(_module->getContext())
          //, llvm::Type::getInt8Ty(_module->getContext()) 
      )
    );
    // auto argAlloca = AllocaValueOfType(builder, arg.getType(), 0, arg_num == 1 && createForCtor ? false: true);
    // auto argValue = builder.CreateLoad(argAlloca);

    auto malloc = builder.CreateCall(mallocForTypeFunc);
    auto mallocOfType = builder.CreateBitCast(malloc, type->getPointerTo());
    auto argValue = builder.CreateLoad(mallocOfType);
    arg_num ++;

    calleeArgs.push_back(argValue);
    // CreatePrintfCall(builder, "arg " + std::to_string(arg_num - 1) + " created.\n");
  }
  // CreatePrintfCall(builder, "args created.\n");
  auto retObj = builder.CreateCall(callee, calleeArgs);
  // CreatePrintfCall(builder, "after call api.\n");

  auto retType = retObj->getType();
  if(retType == builder.getVoidTy()) {
    auto voidRetPtr = CreateMallocCall(builder, builder.getInt64Ty());
    builder.CreateStore(builder.getInt64(0), voidRetPtr);
    if(createForCtor == false) {
      CreatePrintfCall(builder, "retVoidType:\tsave to object list.\n");
      CreateSaveToObjectListCall(builder, builder.CreatePtrToInt(voidRetPtr, builder.getInt64Ty()));
    }
  } else {
    auto retObjMalloc = CreateMallocCall(builder, retType);
    unsigned TypeSize = _module->getDataLayout().getTypeAllocSize(retType);
      if (llvm::isa<llvm::StructType>(retType)) {
        llvm::StructType *ST = llvm::cast<llvm::StructType>(retType);
        TypeSize = _module->getDataLayout().getStructLayout(ST)->getSizeInBytes();
      }
    builder.CreateStore(retObj, retObjMalloc);
    if(createForCtor == false) { // this callee is not a ctor, and this dispatchfuncion is not for invoke ctors.
      CreatePrintfCall(builder, "retType:\tsave to object list.\n");
      CreateSaveToObjectListCall(builder, builder.CreatePtrToInt(retObjMalloc, builder.getInt64Ty()));
    }
  }

  llvm::Value* retObjPtr;
  if(createForCtor == false) {
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
    const std::vector<Either<const llvm::Function *, LLVMFunctionSignature>>& candidates) {
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
      func = const_cast<llvm::Function *>(iter.AsLhs());
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

  // add attribute: optnone unwind
  {
    llvm::AttrBuilder B;
    B.addAttribute(llvm::Attribute::NoInline);
    B.addAttribute(llvm::Attribute::OptimizeNone);
    B.addAttribute(llvm::Attribute::UWTable);
    callbackFuncArrDispatch->addAttributes(llvm::AttributeList::FunctionIndex, B);
  }
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
    // auto retValueAddr = builder.CreateAlloca(retType);

    // // auto retValueAddr = AllocaValueOfType(builder, retType, 0, false);
    // auto retValue = builder.CreateLoad(retValueAddr);
    // builder.CreateRet(retValue);

    auto randomBytesValue = CreateGetRandomBytesCall(
      builder, (retType->getScalarSizeInBits() >> 3));
    auto retAddr = builder.CreateBitCast(randomBytesValue, retType->getPointerTo());
    auto retValue = builder.CreateLoad(retAddr);
    builder.CreateRet(retValue);
  }

  // add attribute: optnone unwind
  {
    llvm::AttrBuilder B;
    B.addAttribute(llvm::Attribute::NoInline);
    B.addAttribute(llvm::Attribute::OptimizeNone);
    B.addAttribute(llvm::Attribute::UWTable);
    emptyFunc->addAttributes(llvm::AttributeList::FunctionIndex, B);
  }

  return emptyFunc;
}

} // namespace caf
