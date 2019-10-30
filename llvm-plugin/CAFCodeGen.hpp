#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"

#include "CAFSymbolTable.hpp"

#ifndef CAF_RECURSIVE_MAX_DEPTH
#define CAF_RECURSIVE_MAX_DEPTH 16
#endif

namespace {

/**
 * @brief Provide logic for generate CAF code stub.
 * 
 */
class CAFCodeGenerator {
public:
  /**
   * @brief Construct a new CAFCodeGenerator object.
   * 
   */
  explicit CAFCodeGenerator()
    : _module(nullptr),
      _symbols(nullptr)
  { }

  /**
   * @brief Set the context of the code generator.
   * 
   * @param module the module into which the code stub will be generated.
   * @param symbols the symbol table containing target APIs.
   */
  void setContext(
      llvm::Module& module, const CAFSymbolTable& symbols) noexcept {
    _module = &module;
    _symbols = &symbols;
    _apiCounter = 0;
  }

  /**
   * @brief Generate CAF code stub into the context.
   * 
   */
  void generateStub() {
    auto dispatchFunc = generateFunctionDispatcher();

    //
    // ================ lib function declaration. ==================
    //
    // zys: declare the lib function: strcmp

    /*
    Global variables, functions and aliases may have an optional runtime 
    preemption specifier. If a preemption specifier isn’t given explicitly, 
    then a symbol is assumed to be dso_preemptable.

    dso_preemptable
    Indicates that the function or variable may be replaced by a symbol from 
    outside the linkage unit at runtime.
    dso_local
    The compiler may assume that a function or variable marked as dso_local will 
    resolve to a symbol within the same linkage unit. Direct access will be 
    generated even if the definition is not within this compilation unit.    
    */
    auto scanfDecl = llvm::cast<llvm::Function>(
        _module->getOrInsertFunction(
            "scanf",
            llvm::FunctionType::get(
                llvm::IntegerType::getInt32Ty(_module->getContext()), 
                llvm::PointerType::getUnqual(
                    llvm::IntegerType::getInt8Ty(_module->getContext())), 
                true
            )
        ).getCallee()
    );
    scanfDecl->setDSOLocal(true);

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
        ).getCallee()
    );
    printfDecl->setDSOLocal(true);

    //
    // =========== define dispatchFunc: the callee function for fuzzer. =============
    // 

    llvm::Value* zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(
        _module->getContext()), 0);

    
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
        ).getCallee()
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
    // llvm::Value* bufferSize = llvm::ConstantInt::get(
    //     llvm::Type::getInt32Ty(_module->getContext()), 128);
    // llvm::Value* inputBuffer = builder.CreateAlloca(
    //     llvm::IntegerType::getInt8Ty(_module->getContext()), 
    //     bufferSize, 
    //     "input_name");

    llvm::Value* int32Size = llvm::ConstantInt::get(
        llvm::Type::getInt32Ty(_module->getContext()), 1);
    auto inputInt32 = builder.CreateAlloca(
        llvm::IntegerType::getInt32Ty(_module->getContext()), 
        int32Size, 
        "input_id");
    builder.CreateBr(mainWhileCond);

    builder.SetInsertPoint(mainWhileCond);
    {
      std::string scanfFormat("%d");
      auto stringConstant = llvm::ConstantDataArray::getString(
          newMain->getContext(), scanfFormat);
      llvm::Value* stringVar = builder.CreateAlloca(stringConstant->getType());
      builder.CreateStore(stringConstant, stringVar);
      llvm::Value* formatStr = builder.CreatePointerCast(
          stringVar, builder.getInt8PtrTy());
      llvm::Value* scanfParams[] = { formatStr, inputInt32 };
      auto scanfRes = builder.CreateCall(scanfDecl, scanfParams);
      llvm::Value* scanfCond = builder.CreateICmpNE(scanfRes, zero);
      builder.CreateCondBr(scanfCond, mainWhileBody, mainEnd);
    }


    builder.SetInsertPoint(mainWhileBody);
    {
      std::string printfFormat("%d\n");
      llvm::Constant* stringConstant = llvm::ConstantDataArray::getString(
          newMain->getContext(), printfFormat);
      llvm::Value* stringVar = builder.CreateAlloca(stringConstant->getType());
      builder.CreateStore(stringConstant, stringVar);
      llvm::Value* formatStr = builder.CreatePointerCast(
          stringVar, builder.getInt8PtrTy());
      llvm::Value* loadIdValue = builder.CreateLoad(inputInt32);
      llvm::Value* printfParams[] = { formatStr, loadIdValue };
      builder.CreateCall(printfDecl, printfParams);
      builder.CreateCall(dispatchFunc, loadIdValue);
    }
    builder.CreateBr(mainWhileCond);

    builder.SetInsertPoint(mainEnd);
    builder.CreateRet(zero);
  }

private:
  llvm::Module* _module;
  const CAFSymbolTable* _symbols;
  int _apiCounter;

  /**
   * @brief Generate the dispatch function in the module.
   * 
   * @return llvm::Function* the function generated.
   */
  llvm::Function* generateFunctionDispatcher() noexcept {
    auto dispatchFunc = llvm::cast<llvm::Function>(
        _module->getOrInsertFunction(
            "__caf_dispatch",
            llvm::Type::getVoidTy(_module->getContext()),
            llvm::Type::getInt32Ty(_module->getContext())
        ).getCallee()
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
    for (const auto& func: _symbols->apis()) {
      cases.push_back(generateCallApi(func, dispatchFunc));
    }

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

  /**
   * @brief Test whether instances of the given type can be allocated on the
   * stack.
   * 
   * @param type the type of the instance to be allocated.
   * @return true if instances of the given type can be allocated on the stack.
   * @return false if instances of the given type cannot be allocated on the
   * stack.
   */
  bool canAlloca(const llvm::Type* type) {
    if (type->isStructTy()) {
      auto structType = llvm::dyn_cast<llvm::StructType>(type);
      return !structType->isOpaque();
    } else {
      return true;
    }
  }

  /**
   * @brief Allocate a value of a struct type.
   * 
   * @param type the struct type.
   * @param builder IR builder.
   * @param depth depth of the current generator.
   * @return llvm::Value* generated value.
   */
  llvm::Value* allocaStructValue(
      llvm::StructType* type, llvm::IRBuilder<> builder, int depth) {
    llvm::Value* argAlloca = builder.CreateAlloca(type);
    auto stype = llvm::dyn_cast<llvm::StructType>(type);

    if (stype->isLiteral()) {
      std::string tname = type->getStructName().str();
      auto found = tname.find("."); //class.basename / struct.basename
      std::string basename = tname.substr(found + 1);

      // TODO: Refactor here to choosse different activators.
      if (_symbols->getConstructor(basename)) {
        auto ctor = (*_symbols->getConstructor(basename))[0];
        std::vector<llvm::Value *> ctorParams { };
        llvm::Value* thisAlloca = nullptr;
        for (const auto& arg: ctor->args()) {
          if (!thisAlloca) {
            thisAlloca = argAlloca;
            ctorParams.push_back(thisAlloca);
            continue;
          }
          auto argType = arg.getType();
          auto argTypeAlloca = allocaValueOfType(
              argType, builder, depth + 1);
          ctorParams.push_back(builder.CreateLoad(argTypeAlloca));
        }
        builder.CreateCall(ctor, ctorParams);
      } else if (_symbols->getFactoryFunction(basename)) {
        auto func = (*_symbols->getFactoryFunction(basename))[0];
        auto retType = func->getReturnType();
        llvm::Value* argAlloca = builder.CreateCall(func);
        if (retType->isStructTy()) {
          argAlloca = builder.CreateIntToPtr(
              argAlloca, retType->getPointerTo(0));
        }
      } else {
        // No ctor found for the type. Initialize each field manually.
        auto elements = static_cast<int>(stype->getNumElements());
        for (auto id = 0; id < elements; id++) {
          auto elementType = stype->getElementType(id);
          auto elementAlloca = allocaValueOfType(
              elementType, builder, depth + 1);
          auto elementValue = builder.CreateLoad(elementAlloca);

          // auto vid = llvm::ConstantInt::get(
          //     llvm::Type::getInt32Ty(builder.getContext()), id);
          auto elementptr = builder.CreateStructGEP(argAlloca, id);
          builder.CreateStore(elementValue, elementptr);
        }
      }
    }

    return argAlloca;
  }

  /**
   * @brief Allocate a value of a pointer type.
   * 
   * @param type the pointer type.
   * @param builder IR builder.
   * @param depth depth of the current generator.
   * @return llvm::Value* allocated value.
   */
  llvm::Value* allocaPointerType(
      llvm::PointerType* type, llvm::IRBuilder<> builder, int depth = 0) {
    auto argAlloca = builder.CreateAlloca(type); // 按类型分配地址空间
    llvm::Type* pointeeType = type->getPointerElementType();

    if (depth >= CAF_RECURSIVE_MAX_DEPTH || !canAlloca(pointeeType)) {
      auto nil = llvm::ConstantPointerNull::get(
          llvm::dyn_cast<llvm::PointerType>(type));
      builder.CreateStore(nil, argAlloca);
    } else {
      auto pointeeValue = allocaValueOfType(
          pointeeType, builder, depth + 1);
      builder.CreateStore(pointeeValue, argAlloca);
    }

    return argAlloca;
  }

  /**
   * @brief Allocate a value of an array type.
   * 
   * @param type the type to allocate.
   * @param builder IR builder.
   * @param depth depth of the generator.
   * @return llvm::Value* the allocated value.
   */
  llvm::Value* allocaArrayType(
      llvm::ArrayType* type, llvm::IRBuilder<> builder, int depth = 0) {
    // auto arraySize = llvm::ConstantInt::get(
    //     llvm::Type::getInt32Ty(builder.getContext()), type->getNumElements());
    // auto element_type = type->getElementType();
    // TODO: Add code here to allocate elements of the array.
    return builder.CreateAlloca(type);
  }

  /**
   * @brief Allocate a value of a function type.
   * 
   * @param type the type to allocate.
   * @param builder IR builder.
   * @param depth depth of the generator.
   * @return llvm::Value* the allocated value.
   */
  llvm::Value* allocaFunctionType(
      llvm::FunctionType* type, llvm::IRBuilder<> builder, int depth = 0) {
    // TODO: Add code here to allocate a value of a function type.
    auto pointerType = type->getPointerTo();
    return builder.CreateLoad(builder.CreateAlloca(pointerType));
  }

  /**
   * @brief Allocate a value of the given type.
   * 
   * @param type the type to allocate.
   * @param builder IR builder.
   * @param depth depth of the generator.
   * @return llvm::Value* the allocated value.
   */
  llvm::Value* allocaValueOfType(
      llvm::Type* type, llvm::IRBuilder<> builder, int depth = 0) {
    if (type->isIntegerTy() || type->isFloatingPointTy()) {
      return builder.CreateAlloca(type);
    } else if (type->isStructTy()) {
      return allocaStructValue(
          llvm::dyn_cast<llvm::StructType>(type), builder, depth);
    } else if (type->isPointerTy()) {
      return allocaPointerType(
          llvm::dyn_cast<llvm::PointerType>(type), builder, depth);
    } else if (type->isArrayTy()) {
      return allocaArrayType(
          llvm::dyn_cast<llvm::ArrayType>(type), builder, depth);
    } else if (type->isFunctionTy()) {
      return allocaFunctionType(
          llvm::dyn_cast<llvm::FunctionType>(type), builder, depth);
    } else {
      llvm::errs() << "Trying to alloca unrecognized type: "
          << type->getTypeID() << "\n";
      return nullptr;
    }
  }

  std::pair<llvm::ConstantInt *, llvm::BasicBlock *> generateCallApi(
      llvm::Function* callee, llvm::Function* caller) {
    llvm::IRBuilder<> builder { caller->getContext() };
    auto module = caller->getParent();
    auto& context = module->getContext();
    // const auto& DL = module->getDataLayout();

    auto printfDecl = llvm::cast<llvm::Function>(
        module->getOrInsertFunction(
            "printf",
            llvm::FunctionType::get(
                llvm::IntegerType::getInt32Ty(context), 
                llvm::PointerType::getUnqual(
                    llvm::IntegerType::getInt8Ty(context)
                ),
                true
            )
        ).getCallee()
    );
    printfDecl->setDSOLocal(true);

    std::string block_name("invoke.api.case.");
    block_name.append(std::to_string(_apiCounter));
    auto invokeApiCase = llvm::BasicBlock::Create(
        caller->getContext(), block_name, caller);
    
    builder.SetInsertPoint(invokeApiCase);
    std::string printfFormat = callee->getName().str();
    printfFormat.push_back('\n');
    auto stringConstant = llvm::ConstantDataArray::getString(
        caller->getContext(), printfFormat);
    auto stringVar = builder.CreateAlloca(stringConstant->getType());
    builder.CreateStore(stringConstant, stringVar);
    auto formatStr = builder.CreatePointerCast(
        stringVar, builder.getInt8PtrTy());

    llvm::Value* printf_params[] = { formatStr };
    builder.CreateCall(printfDecl, printf_params);

    std::vector<llvm::Value *> calleeArgs { };
    for (const auto& arg: callee->args()) {
      auto argAlloca = allocaValueOfType(arg.getType(), builder);
      auto argValue = builder.CreateLoad(argAlloca);

      calleeArgs.push_back(argValue);
    }
    builder.CreateCall(callee, calleeArgs);

    auto caseId = llvm::ConstantInt::get(
        llvm::Type::getInt32Ty(context), _apiCounter);
    _apiCounter++;
    return std::make_pair(caseId, invokeApiCase);
  }
};

} // namespace <anonymous>
