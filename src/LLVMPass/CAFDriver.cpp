#include <utility>
#include <iterator>
#include <system_error>
#include <string>
#include <unordered_map>
#include <algorithm>

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

#include "Basic/CAFStore.h"
#include "Basic/JsonSerializer.h"
#include "SymbolTable.h"
#include "CodeGen.h"
#include "ABI.h"

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
 * @brief Removes template arguments and function arguments specified in the given symbol name.
 *
 * Formally, this function returns any contents that is surrounded by angle brackets or round
 * brackets in some suffix of the given string.
 *
 * @param name the original symbol name.
 * @return std::string the symbol name with template arguments and function arguments removed.
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
   * @brief Get the component at the given index. The index can be negative in which case the index
   * will counts from the back of the container.
   *
   * @param index the index.
   * @return const std::string& the component at the given index.
   */
  const std::string& operator[](int index) const {
    if (index >= 0) {
      return _components[index];
    } else {
      return _components[_components.size() + index];
    }
  }

private:
  std::vector<std::string> _components;

  /**
   * @brief Load the given qualified symbol name and split all components, with regard to the
   * necessary bracket structure.
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
  symbol::QualifiedName name { caf::demangle(fn.getName().str()) };
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
 * @brief Get the name of the type constructed by the given constructor. If the given function is
 * not a constructor, the behavior is unspecified.
 *
 * @param ctor reference to a llvm::Function object representing a constructor.
 * @return std::string the name of the type constructed by the given constructor.
 */
std::string getConstructedTypeName(const llvm::Function& ctor) {
  symbol::QualifiedName ctorName { caf::demangle(ctor.getName().str()) };
  // The demangled name of the constructor should be something like
  // `[<namespace>::]<className>::<funcName>`. We're interested in
  // `[<namespace>::]<className>`.
  return ctorName[-2];
}

/**
 * @brief Get the type name of the given factory function. If the given function is not a factory
 * function, the behavior is unspecified.
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
  // TODO: Uncomment the following if statement after the CafApi attribute patch has been applied.
  // if(fn.hasFnAttribute(llvm::Attribute::CafApi)) {
  //   fn.dump();
  //   return true;
  // }

  if (fn.arg_size() != 1) {
    return false;
  }

  auto onlyArg = fn.args().begin();
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

  return onlyArgStructType->getName().str() == "class.v8::FunctionCallbackInfo";
}

/**
 * @brief Get a list of functions that are directly called by the given
 * function.
 *
 * @param fn the caller function.
 * @return std::vector<const llvm::Function *> a list of functions that are
 * directly called by the given function.
 */
std::vector<llvm::Function *> getFunctionCallees(llvm::Function& fn) {
  std::vector<llvm::Function *> callees { };

  for (const auto& basicBlock : fn)
  for (const auto& instruction : basicBlock) {
    llvm::Function* callee = nullptr;
    if (llvm::isa<llvm::CallInst>(instruction)) {
      callee = llvm::dyn_cast<llvm::CallInst>(&instruction)->getCalledFunction();
    } else if (llvm::isa<llvm::InvokeInst>(instruction)) {
      callee = llvm::dyn_cast<llvm::InvokeInst>(&instruction)->getCalledFunction();
    }

    if (callee) {
      callees.push_back(callee);
    }
  }

  return callees;
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
    populateSymbolTable(module);

    auto symbolTableFreeze = _symbols.Freeze();
    saveCAFStore(symbolTableFreeze.store.get());

    _codeGen.SetContext(module, _symbols);
    _codeGen.GenerateCallbackFunctionCandidateArray(symbolTableFreeze.callbackFunctions);
    _codeGen.GenerateStub();

    return false;
  }

private:
  // Symbol table.
  caf::CAFSymbolTable _symbols;
  caf::CAFCodeGenerator _codeGen;

  /**
   * @brief Extracts all `interesting` functions from the given LLVM module and add them to the
   * symbol table.
   *
   * A function is considered `interesting` if any of the following holds:
   *
   * * The function is the constructor of some type;
   * * The function is a factory function, a.k.a. functions whose arg list is empty and returns an
   * instance of some type;
   * * The function takes exactly one argument of type `v8::FunctionCallbackInfo`, in which case the
   * function is called a top-level API. Top-level API functions are the direct target of fuzzing,
   * like syscalls in syzkaller.
   *
   * The extracted constructors, factory functions and top-level APIs are populated into the
   * corresponding private fields of this class.
   *
   * @param module The LLVM module to extract functions from.
   */
  void populateSymbolTable(llvm::Module& module) {
    // TODO: This function need to be refactored to allow end users to define
    // TODO: custom filters for top-level APIs.
    for (auto& func : module) {
      _symbols.AddCallbackFunctionCandidate(&func);

      auto funcName = caf::demangle(func.getName().str());
      // funcName = removeParentheses(funcName);

      if (isConstructor(func)) {
        // This function is a constructor.
        auto typeName = getConstructedTypeName(func);
        _symbols.AddConstructor(typeName, &func);
        llvm::errs() << "CAF: found constructor: " << func.getName().str()
            << " for type: " << typeName
            << "\n";
      } else if (isTopLevelApi(func)) {
        // This function is a top-level API.
        // _symbols.addApi(&func);
        llvm::errs() << "CAF: found top-level API: " << func.getName().str()
            << "\n";
        for (auto targetApi : getFunctionCallees(func)) {
          llvm::errs() << "CAF: found fuzz-target API: "
              << targetApi->getName().str() << "\n";
          _symbols.AddApi(targetApi);
        }
        _symbols.AddApi(&func);
      }
    }
  }

  void saveCAFStore(const caf::CAFStore* store) {
    caf::JsonSerializer jsonSerializer { };
    auto json = jsonSerializer.Serialize(*store);

    // TODO: Refactor here to allow user specify the path to save CAF store.
    auto jsonText = json.dump();
    std::error_code openDumpFileError { };
    llvm::raw_fd_ostream dumpFile {
        "/home/zys/Temp/caf.json", openDumpFileError };

    if (openDumpFileError) {
      llvm::errs() << "CAFDriver: failed to dump metadata to file: "
          << openDumpFileError.value() << ": "
          << openDumpFileError.message() << "\n";
      return;
    }

    dumpFile << jsonText;
    dumpFile.flush();
    dumpFile.close();

    llvm::errs() << "CAF metadata saved to " << "/home/zys/Temp/caf.json" << "."
        << "\n";
  }

  void init() {
    _symbols.clear();
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
