#include "llvm/Pass.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "llvm/IR/PassManager.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/IRBuilder.h"

#include <cxxabi.h>
#include <utility>
#include <iterator>
#include <string>
#include <unordered_map>
#include <algorithm>


namespace {

namespace utils {

template <typename K, typename V, typename ValueFactory, typename ValueUpdator>
void insertOrUpdate(std::unordered_map<K, V>& um, K&& key,
                    ValueFactory factory,
                    ValueUpdator updator) {
  auto i = um.find(key);
  if (i == um.end()) {
    um.emplace(std::forward<K>(key), factory());
  } else {
    updator(i->second);
  }
}

} // namespace utils

namespace symbol {

/**
 * @brief Demangle symbol names.
 * 
 * @param name The mangled symbol name.
 * @return std::string The demangled symbol name. If the given symbol name cannot
 * be successfully demangled, returns the original name.
 */
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
 * @brief Removes template arguments and function arguments specified in the 
 * given symbol name.
 * 
 * Formally, this function returns any contents that is surrounded by angle 
 * brackets or round brackets in some suffix of the given string.
 * 
 * @param name the original symbol name.
 * @return std::string the symbol name with template arguments and function 
 * arguments removed.
 */
std::string removeArgs(const std::string& name) {
  if (name.empty()) {
    return std::string { };
  }
  
  if (name.back() != ')' && name.back() != '>') {
    return name;
  }

  auto counter = 0;
  for (auto i = name.rbegin(); i != name.rend(); ++i) {
    if (*i == ')' || *i == '>') {
      ++counter;
    } else if (*i == '(' || *i == '<') {
      --counter;
    }

    if (!counter) {
      return name.substr(0, std::distance(i, name.rend()) - 1);
    }
  }

  // Rarely should here be reached. Return the original name of here is actually
  // reached.
  return name;
}

/**
 * @brief Represent a qualified name.
 * 
 * A qualified name is a symbol name whose components are separated by "::".
 * 
 */
class QualifiedName {
public:
  /**
   * @brief Construct a new QualifiedName object.
   * 
   * @param name the original qualified name.
   */
  explicit QualifiedName(const std::string& name)
    : _components { } {
    load(name);
  }

  using iterator = typename std::vector<std::string>::const_iterator;
  using size_type = typename std::vector<std::string>::size_type;

  iterator begin() const {
    return _components.begin();
  }

  iterator end() const {
    return _components.end();
  }

  /**
   * @brief Get the number of components contained in the fully qualified name.
   * 
   * @return size_type the number of components.
   */
  size_type size() const {
    return _components.size();
  }

  /**
   * @brief Get the component at the given index. The index can be negative
   * in which case the index will counts from the back of the container.
   * 
   * @param index the index.
   * @return const std::string& the component at the given index.
   */
  const std::string& operator[](int index) {
    if (index >= 0) {
      return _components[index];
    } else {
      return _components[_components.size() + index];
    }
  }

private:
  std::vector<std::string> _components;

  /**
   * @brief Load the given qualified symbol name and split all components, with
   * regard to the necessary bracket structure.
   * 
   * @param name the qualified symbol name.
   */
  void load(const std::string& name) {
    for (auto left = 0; left < static_cast<int>(name.length()); ) {
      auto bracketDepth = 0;
      auto right = left;
      while (right < static_cast<int>(name.length())) {
        if (right + 1 < static_cast<int>(name.length()) &&
            bracketDepth == 0 &&
            name[right] == ':' && name[right + 1] == ':') {
          // Split point hit.
          break;
        }

        auto curr = name[right++];
        if (curr == '(' || curr == '<' || curr == '[' || curr == '{') {
          ++bracketDepth;
        } else if (curr == ')' || curr == '>' || curr == ']' || curr == ')') {
          --bracketDepth;
        }
      }

      _components.emplace_back(name.substr(left, right - left));
      left = right + 2; // Skip the "::" separator.
    }
  }
};

} // namespace symbol

/**
 * @brief Test whether the given function is a constructor.
 * 
 * @param fn The function to test.
 * @return true if the function given is a constructor.
 * @return false if the function given is not a constructor.
 */
bool isConstructor(const llvm::Function& fn) {
  symbol::QualifiedName name { symbol::demangle(fn.getName().str()) };
  // The demangled name of a constructor should be something like
  // `[<namespace>::]<className>::<funcName>`. So to be a constructor,
  // there should be at least 2 components in the fully qualified name.
  if (name.size() < 2) {
    return false;
  }

  // The function name without function arguments should equal to the class name 
  // without template arguments if `fn` is a constructor.
  auto funcNameWithoutArgs = symbol::removeArgs(name[-1]);
  auto classNameWithoutArgs = symbol::removeArgs(name[-2]);
  return funcNameWithoutArgs == classNameWithoutArgs;
}

/**
 * @brief Get the name of the type constructed by the given constructor. If the
 * given function is not a constructor, the behavior is unspecified.
 * 
 * @param ctor reference to a llvm::Function object representing a constructor.
 * @return std::string the name of the type constructed by the given
 * constructor.
 */
std::string getConstructedTypeName(const llvm::Function& ctor) {
  symbol::QualifiedName ctorName { symbol::demangle(ctor.getName().str()) };
  // The demangled name of the constructor should be something like
  // `[<namespace>::]<className>::<funcName>`. We're interested in 
  // `[<namespace>::]<className>`.
  return ctorName[-2];
}

/**
 * @brief Test whether the given function is a factory function.
 * 
 * Factory functions meets the following conditions:
 * 
 * * It takes no arguments;
 * * It returns a value of some struct or class type.
 * 
 * @param fn The function to test.
 * @return true if the function given is a factory function.
 * @return false if the function given is not a factory function.
 */
bool isFactoryFunction(const llvm::Function& fn) {
  if (fn.arg_size()) {
    return false;
  }

  auto retType = fn.getReturnType();
  // If the return type is a pointer type, decay it to the pointee's type.
  if (retType->isPointerTy()) {
    retType = retType->getPointerElementType();
  }

  if (retType->isStructTy()) {
    auto structType = llvm::dyn_cast<llvm::StructType>(retType);
    return !structType->isLiteral();
  }

  return false;
}

/**
 * @brief Get the type name of the given factory function. If the given function
 * is not a factory function, the behavior is unspecified.
 * 
 * @param factoryFunction the factory function.
 * @return std::string the type name of the given factory function,
 */
std::string getProducingTypeName(const llvm::Function& factoryFunction) {
  auto retType = factoryFunction.getReturnType();
  if (retType->isPointerTy()) {
    retType = retType->getPointerElementType();
  }
  auto structType = llvm::dyn_cast<llvm::StructType>(retType);
  auto structName = structType->getStructName().str();
  // The name of structs inside LLVM is shaped like class.<name> or
  // struct.<name>. We only need the <name> part.
  return structName.substr(structName.find(".") + 1);
}

/**
 * @brief Test whether the given function is a top-level API function.
 * 
 * A function is called a top-level API if it takes a single argument of type
 * `v8::FunctionCallbackInfo *`.
 * 
 * @param fn the function to test.
 * @return true if the function given is a top-level API function.
 * @return false if the function given is not a top-level API function.
 */
bool isTopLevelApi(const llvm::Function& fn) {
  // TODO: Refactor to let end users customize top-level API filter logic.
  if (fn.arg_size() != 1) {
    return false;
  }

  auto onlyArg = fn.getArg(0);
  auto onlyArgType = onlyArg->getType();
  if (!onlyArgType->isPointerTy()) {
    return false;
  }

  onlyArgType = onlyArgType->getPointerElementType();
  if (!onlyArgType->isStructTy()) {
    return false;
  }

  auto onlyArgStructType = llvm::dyn_cast<llvm::StructType>(onlyArgType);
  if (onlyArgStructType->isLiteral()) {
    return false;
  }

  return onlyArgStructType->getName().str() == "v8::FunctionCallbackInfo";
}

/**
 * @brief Implement an LLVM module pass that extracts API and type information 
 * of the target.
 * 
 */
class CAFDriver : public llvm::ModulePass {
public:
  /**
   * @brief This field is used as an alternative machenism to RTTI in LLVM.
   * 
   */
  static char ID;

  /**
   * @brief Construct a new CAFDriver object.
   */
  explicit CAFDriver()
    : ModulePass(ID)
  { }

  /**
   * @brief This function is the entry point of the module pass.
   * 
   * @param module The LLVM module to work on.
   * @return true 
   * @return false 
   */
  bool runOnModule(llvm::Module& module) override {
    init();

    // Extract interesting module functions and populates them into the private
    // fields of this class.
    extractModuleFunctions(module);

    llvm::Function* call_me_to_invoke_api = gen_call_me_to_invoke_api(module);

    //
    // ================ lib function declaration. ==================
    //
    //zys: declare the lib function: strcmp

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
    llvm::Function* scanf_declare = llvm::cast<llvm::Function>(
        module.getOrInsertFunction(
            "scanf",
            llvm::FunctionType::get(
                llvm::IntegerType::getInt32Ty(module.getContext()), 
                llvm::PointerType::getUnqual(
                    llvm::IntegerType::getInt8Ty(module.getContext())), 
                true
            )
        ).getCallee()
    );
    scanf_declare->setDSOLocal(true);

    llvm::Function* printf_declare = llvm::cast<llvm::Function>(
        module.getOrInsertFunction(
            "printf",
            llvm::FunctionType::get(
                llvm::IntegerType::getInt32Ty(module.getContext()),
                llvm::PointerType::getUnqual(
                    llvm::IntegerType::getInt8Ty(module.getContext())
                ),
                true
            )
        ).getCallee()
    );
    printf_declare->setDSOLocal(true);

    //
    // =========== define call_me_to_invoke_api: the callee function for fuzzer. =============
    // 

    llvm::Value* zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(
        module.getContext()), 0);

    
    //
    // ============= insert my new main function. =======================
    //

    // delete the old main function.
    llvm::Function* old_main = module.getFunction("main");
    if (old_main) {
      old_main->eraseFromParent();
    }

    llvm::Function* new_main = llvm::cast<llvm::Function>(
        module.getOrInsertFunction(
            "main", 
            llvm::IntegerType::getInt32Ty(module.getContext()),
            llvm::IntegerType::getInt32Ty(module.getContext()),
            llvm::PointerType::getUnqual(
                llvm::PointerType::getUnqual(
                    llvm::IntegerType::getInt8Ty(module.getContext())
                )
            )
        ).getCallee()
    );

    {
      new_main->setCallingConv(llvm::CallingConv::C);
      auto args = new_main->arg_begin();
      llvm::Value* arg_0 = &*args++;
      arg_0->setName("argc");
      llvm::Value* arg_1 = &*args++;
      arg_1->setName("argv");
    }

    llvm::BasicBlock* main_entry = llvm::BasicBlock::Create(
        new_main->getContext(), "main.entry", new_main);
    llvm::BasicBlock* main_while_cond = llvm::BasicBlock::Create(
      new_main->getContext(), "main.while.cond", new_main);
    llvm::BasicBlock* main_while_body = llvm::BasicBlock::Create(
      new_main->getContext(), "main.while.body", new_main);
    llvm::BasicBlock* main_end = llvm::BasicBlock::Create(
      new_main->getContext(), "main.end", new_main);

    llvm::IRBuilder<> builder { new_main->getContext() };
    builder.SetInsertPoint(main_entry);
    llvm::Value* buffer_size = llvm::ConstantInt::get(
        llvm::Type::getInt32Ty(module.getContext()), 128);
    llvm::Value* input_buffer = builder.CreateAlloca(
        llvm::IntegerType::getInt8Ty(module.getContext()), 
        buffer_size, 
        "input_name");

    llvm::Value* int32_size = llvm::ConstantInt::get(
        llvm::Type::getInt32Ty(module.getContext()), 1);
    llvm::AllocaInst* input_int32 = builder.CreateAlloca(
        llvm::IntegerType::getInt32Ty(module.getContext()), 
        int32_size, 
        "input_id");
    builder.CreateBr(main_while_cond);

    builder.SetInsertPoint(main_while_cond);
    {
      std::string scanf_format { "%d" };
      llvm::Constant* stringConstant = llvm::ConstantDataArray::getString(
          new_main->getContext(), scanf_format);
      llvm::Value* stringVar = builder.CreateAlloca(stringConstant->getType());
      builder.CreateStore(stringConstant, stringVar);
      llvm::Value* format_str = builder.CreatePointerCast(
          stringVar, builder.getInt8PtrTy());
      llvm::Value* scanf_params[] = { format_str, input_int32 };
      llvm::CallInst * scanf_res = builder.CreateCall(
          scanf_declare, scanf_params);
      llvm::Value* scanf_cond = builder.CreateICmpNE(scanf_res, zero);
      builder.CreateCondBr(scanf_cond, main_while_body, main_end);
    }


    builder.SetInsertPoint(main_while_body);
    {
      std::string printf_format { "%d\n" };
      llvm::Constant* stringConstant = llvm::ConstantDataArray::getString(
          new_main->getContext(), printf_format);
      llvm::Value* stringVar = builder.CreateAlloca(stringConstant->getType());
      builder.CreateStore(stringConstant, stringVar);
      llvm::Value* format_str = builder.CreatePointerCast(
          stringVar, builder.getInt8PtrTy());
      llvm::Value* load_idvalue = builder.CreateLoad(input_int32);
      llvm::Value* printf_params[] = { format_str, load_idvalue };
      llvm::CallInst* printf_res = builder.CreateCall(
          printf_declare, printf_params);

      builder.CreateCall(call_me_to_invoke_api, load_idvalue);
    }
    builder.CreateBr(main_while_cond);

    builder.SetInsertPoint(main_end);
    builder.CreateRet(zero);

    return false;
  }

private:
  // Top-level APIs.
  std::vector<llvm::Function *> _apis;

  // List of constructors.
  std::unordered_map<std::string, std::vector<llvm::Function *>> _ctors;
  // List of factory functions, a.k.a. functions whose arg list is empty and returns
  // an instance of some type.
  std::unordered_map<std::string, std::vector<llvm::Function *>> _factories;

  std::vector<std::pair<llvm::ConstantInt *, llvm::BasicBlock *>> _cases;
  int api_counter = 0;

  /**
   * @brief Extracts all `interesting` functions from the given LLVM module.
   * 
   * A function is considered `interesting` if any of the following holds:
   * 
   * * The function is the constructor of some type;
   * * The function is a factory function, a.k.a. functions whose arg list is
   * empty and returns an instance of some type;
   * * The function takes exactly one argument of type `v8::FunctionCallbackInfo`,
   * in which case the function is called a top-level API. Top-level API 
   * functions are the direct target of fuzzing, like syscalls in syzkaller.
   * 
   * The extracted constructors, factory functions and top-level APIs are
   * populated into the corresponding private fields of this class.
   * 
   * @param module The LLVM module to extract functions from.
   */
  void extractModuleFunctions(llvm::Module& module) {
    // TDOO: This function need to be refactored to allow end users to define
    // custom filters for top-level APIs.
    for (auto& func : module) {
      auto funcName = symbol::demangle(func.getName().str());
      // funcName = removeParentheses(funcName);

      if (isConstructor(func)) {
        auto typeName = getConstructedTypeName(func);

        // Insert (typeName, &func) into _ctors.
        utils::insertOrUpdate(_ctors, std::move(typeName),
            [&func] () -> auto { 
              return std::vector<llvm::Function *> { &func };
            },
            [&func] (std::vector<llvm::Function *>& old) {
              old.push_back(&func);
            });
      } else if (isFactoryFunction(func)) {
        // Add the function to factory function table.
        auto structName = getProducingTypeName(func);

        // Insert (structName, func) into _factories.
        utils::insertOrUpdate(_factories, std::move(structName),
            [&func] () -> auto {
              return std::vector<llvm::Function *> { &func };
            },
            [&func] (std::vector<llvm::Function *>& old) {
              old.push_back(&func);
            });
      } else if (isTopLevelApi(func)) {
        _apis.push_back(&func);
      }
    }
  }

  void init() {
    _apis.clear();
    _ctors.clear();
    _factories.clear();

    _cases.clear();
  }

/*

%"class.v8::FunctionCallbackInfo" = type <{ %"class.v8::internal::Object"**, %"class.v8::internal::Object"**, i32, [4 x i8] }>


*/ // return a pointer of memory of 'type'.
  llvm::Value * alloca_param_with_type(
      llvm::Type * type, llvm::IRBuilder<> builder, int depth = 0) {

    llvm::Value* arg_alloca;
    if (type->isStructTy()) {
      // 先分配内存
      arg_alloca = builder.CreateAlloca(type);
      llvm::StructType* stype = llvm::dyn_cast<llvm::StructType>(type);
      if (stype->isLiteral()) {
        std::string tname = type->getStructName().str();
        auto found = tname.find("."); //class.basename / struct.basename
        std::string basename = tname.substr(found + 1);

        if (_ctors.find(basename) != _ctors.end()) {
          const std::vector<llvm::Function *>& ctors = _ctors[basename];
          llvm::Function* ctor = ctors[0];
          ctor->dump();
          std::vector<llvm::Value *> ctor_params { };
          llvm::Value* this_alloca = nullptr;
          for (llvm::Argument& arg: ctor->args()) {
            if (!this_alloca) {
              this_alloca = arg_alloca;
              ctor_params.push_back(this_alloca);
              continue;
            }
            llvm::Type* arg_type = arg.getType();
            llvm::Value* arg_type_alloca = alloca_param_with_type(
                arg_type, builder, depth+1);
            ctor_params.push_back(builder.CreateLoad(arg_type_alloca));
          }
          builder.CreateCall(ctor, ctor_params);

        } else if(_factories.find(basename) != _factories.end()) {
          std::vector<llvm::Function *>& funcList = _factories[basename];
          llvm::Function* func = funcList[0];
          func->dump();
          llvm::Type* rettype = func->getReturnType();
          llvm::Value* arg_alloca = builder.CreateCall(func);
          if (rettype->isStructTy()) {
            arg_alloca = builder.CreateIntToPtr(
                arg_alloca, rettype->getPointerTo(0));
          }
        } else {
          // No ctor found for the type.
          // 对每个element，分别初始化，然后赋值给父元素
          auto argtype_tot = static_cast<int>(stype->getNumElements());
          for (auto id = 0; id < argtype_tot; id++) {
            llvm::Type* element_type = stype->getElementType(id);
            llvm::Value* element_alloca = alloca_param_with_type(
                element_type, builder, depth + 1);
            llvm::Value* element_value = builder.CreateLoad(element_alloca);

            llvm::Value* vid = llvm::ConstantInt::get(
                llvm::Type::getInt32Ty(builder.getContext()), id);
            llvm::Value* elementptr = builder.CreateStructGEP(arg_alloca, id);
            builder.CreateStore(element_value, elementptr);
          }
        }
      }
    } else if (type->isPointerTy()) {
      arg_alloca = builder.CreateAlloca(type); // 按类型分配地址空间
      llvm::Type * p2type = type->getPointerElementType();

      if (depth >= 16) {
        llvm::Value* nlptr = llvm::ConstantPointerNull::get(
            llvm::dyn_cast<llvm::PointerType>(type));
        builder.CreateStore(nlptr, arg_alloca);
      } else {
        llvm::Value* p2type_alloca = alloca_param_with_type(
            p2type, builder, depth + 1);
        builder.CreateStore(p2type_alloca, arg_alloca);
      }
    } else if (type->isArrayTy()) {
      llvm::ArrayType* atype = llvm::dyn_cast<llvm::ArrayType>(type);
      auto array_size = atype->getNumElements();
      llvm::Value* varray_size = llvm::ConstantInt::get(
          llvm::Type::getInt32Ty(builder.getContext()), array_size);
      llvm::Type* element_type = atype->getElementType();
      arg_alloca = builder.CreateAlloca(type);
    } else if (type->isFunctionTy()) {
      // TODO 
      llvm::FunctionType* ftype = llvm::dyn_cast<llvm::FunctionType>(type);
      ftype->getPointerTo();
      llvm::PointerType* ptype = ftype->getPointerTo();
      ptype->dump();
      arg_alloca = builder.CreateLoad(builder.CreateAlloca(ptype));
    } else {
      arg_alloca = builder.CreateAlloca(type);
    }
    return arg_alloca;
  }

  std::pair<llvm::ConstantInt *, llvm::BasicBlock *> call_this_api(
      llvm::Function* callee, llvm::Function* caller) {
    llvm::IRBuilder<> builder(caller->getContext());
    llvm::Module* module = caller->getParent();
    llvm::LLVMContext& context = module->getContext();
    const llvm::DataLayout& DL = module->getDataLayout();

    llvm::Function* printf_declare = llvm::cast<llvm::Function>(
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
    printf_declare->setDSOLocal(true);

    std::string block_name("invoke.api.case.");
    block_name.append(std::to_string(api_counter));
    llvm::BasicBlock* invoke_api_case = llvm::BasicBlock::Create(
        caller->getContext(), block_name, caller);
    
    builder.SetInsertPoint(invoke_api_case);
    std::string printf_format = callee->getName().str();
    printf_format.push_back('\n');
    llvm::Constant* stringConstant = llvm::ConstantDataArray::getString(
        caller->getContext(), printf_format);
    llvm::Value* stringVar = builder.CreateAlloca(stringConstant->getType());
    builder.CreateStore(stringConstant, stringVar);
    llvm::Value* format_str = builder.CreatePointerCast(
        stringVar, builder.getInt8PtrTy());

    llvm::Value* printf_params[] = { format_str };
    builder.CreateCall(printf_declare, printf_params);

    std::vector<llvm::Value *> callee_args { };
    for (llvm::Argument& arg: callee->args()) {
      // Value *arg_value = Constant::getNullValue(arg.getType()); // 语法上没问题，但是没啥用，传空指针进去干啥
      
      // Value * args_alloca = builder.CreateAlloca(type);
      // Value * arg_value = builder.CreateLoad(args_alloca);

      llvm::Value* arg_alloca = alloca_param_with_type(arg.getType(), builder);
      llvm::Value* arg_value = builder.CreateLoad(arg_alloca);

      callee_args.push_back(arg_value);

    }
    builder.CreateCall(callee, callee_args);

    llvm::ConstantInt* case_id = llvm::ConstantInt::get(
        llvm::Type::getInt32Ty(context), api_counter);
    api_counter++;
    return std::make_pair(case_id, invoke_api_case);
  }

#define _CAF_INSERT_SWITCH_CASE_

  llvm::Function* gen_call_me_to_invoke_api(llvm::Module& module) {
    llvm::Function* call_me_to_invoke_api = llvm::cast<llvm::Function>(
        module.getOrInsertFunction(
            "call_me_to_invoke_api",
            llvm::Type::getVoidTy(module.getContext()),
            llvm::Type::getInt32Ty(module.getContext())
        ).getCallee()
    );
    call_me_to_invoke_api->setCallingConv(llvm::CallingConv::C);

    auto args = call_me_to_invoke_api->arg_begin();
    llvm::Value* api_id = &*args++;
    api_id->setName("api_id");

    llvm::IRBuilder<> builder(call_me_to_invoke_api->getContext());
    llvm::BasicBlock* invoke_api_entry = llvm::BasicBlock::Create(
        call_me_to_invoke_api->getContext(), 
        "invoke.api.entry", 
        call_me_to_invoke_api);

#ifdef _CAF_INSERT_SWITCH_CASE_
    _cases.clear();
    for (auto& func: _apis) {
      _cases.push_back(call_this_api(func, call_me_to_invoke_api));
    }
    
    if (_cases.size() == 0) {
      builder.SetInsertPoint(invoke_api_entry);
      builder.CreateRetVoid();
      return call_me_to_invoke_api;
    }
    
    llvm::BasicBlock* invoke_api_defualt = llvm::BasicBlock::Create(
        call_me_to_invoke_api->getContext(), 
        "invoke.api.defualt", 
        call_me_to_invoke_api);
#endif
    llvm::BasicBlock* invoke_api_end = llvm::BasicBlock::Create(
        call_me_to_invoke_api->getContext(), 
        "invoke.api.end", 
        call_me_to_invoke_api);

#ifdef _CAF_INSERT_SWITCH_CASE_
    for (auto & api_case: _cases) {
      llvm::BasicBlock* bb = api_case.second;
      // builder.SetInsertPoint(&*bb->end());
      builder.SetInsertPoint(bb);
      builder.CreateBr(invoke_api_end);
    }
#endif

    builder.SetInsertPoint(invoke_api_entry);
#ifdef _CAF_INSERT_SWITCH_CASE_
    llvm::SwitchInst* invoke_api_SI = builder.CreateSwitch(
        api_id, _cases.front().second, _cases.size());
    for (auto& c: _cases) {
      invoke_api_SI->addCase(c.first, c.second);
    }
    invoke_api_SI->setDefaultDest(invoke_api_defualt);
    builder.SetInsertPoint(invoke_api_defualt);
#endif
    builder.CreateBr(invoke_api_end);
    builder.SetInsertPoint(invoke_api_end);
    builder.CreateRetVoid();

    call_me_to_invoke_api->getType()->dump();
    return call_me_to_invoke_api;
  }
}; // class CAFDriver

}  // namespace <anonymous>

char CAFDriver::ID = 0;
static llvm::RegisterPass<CAFDriver> X(
    "cafdriver", "CAFDriver Pass",
    false, // Only looks at CFG
    false); // Analysis Pass

static llvm::RegisterStandardPasses Y(
    llvm::PassManagerBuilder::EP_EarlyAsPossible,
    [] (const llvm::PassManagerBuilder &, llvm::legacy::PassManagerBase &m) { 
        m.add(new CAFDriver()); 
    });
