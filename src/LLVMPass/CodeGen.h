#ifndef CAF_CODE_GEN_H
#define CAF_CODE_GEN_H

#include "llvm/IR/IRBuilder.h"

namespace llvm {
class Module;
class CallInst;
} // namespace llvm

namespace caf {

class CAFSymbolTable;

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
      _symbols(nullptr),
      _apiCounter(0)
  { }

  /**
   * @brief Set the context of the code generator.
   *
   * @param module the module into which the code stub will be generated.
   * @param symbols the symbol table containing target APIs.
   */
  void SetContext(llvm::Module& module, const CAFSymbolTable& symbols) {
    _module = &module;
    _symbols = &symbols;
    _apiCounter = 0;
  }

  /**
   * @brief Generate CAF code stub into the context.
   *
   */
  void GenerateStub();

  /**
   * @brief Generate a global array for storing function pointers to the given list of functions.
   *
   * @param candidates the callback function candidates.
   */
  void GenerateCallbackFunctionCandidateArray(const std::vector<llvm::Function *>& candidates);

private:
  llvm::Module* _module;
  const CAFSymbolTable* _symbols;
  int _apiCounter;

  /**
   * @brief Create a call to `printf` to print the given value with the given format string.
   *
   * @param builder the IR builder to use.
   * @param format the format string that will be passed to the `printf` function.
   * @param value the value to print out.
   * @return llvm::CallInst* the created call to `printf` .
   */
  llvm::CallInst* CreatePrintfCall(
      llvm::IRBuilder<>& builder, const std::string& format, llvm::Value* value);

  /**
   * @brief Create a call to `printf` to print the given string.
   *
   * @param builder the IR builder to use.
   * @param str the string to priunt.
   * @return llvm::CallInst* the created call to `printf`.
   */
  llvm::CallInst* CreatePrintfCall(llvm::IRBuilder<>& builder, const std::string& str);

  /**
   * @brief Create a call to `scanf` to read a value from `stdin` into the given @see llvm::Value
   * object.
   *
   * @param builder the IR builder to use.
   * @param format the format string to use.
   * @param dest the @see llvm::Value object for storing the value read from `stdin`.
   * @return llvm::CallInst* the created call to `scanf`.
   */
  llvm::CallInst* CreateScanfCall(
      llvm::IRBuilder<>& builder, const std::string& format, llvm::Value* dest);

  /**
   * @brief Generate the dispatch function in the module.
   *
   * @return llvm::Function* the function generated.
   */
  llvm::Function* CreateDispatchFunction();

  /**
   * @brief Generate code to allocate a value of a struct type on the stack.
   *
   * @param builder the IR builder to use.
   * @param type the struct type of which instances of object will be allocated.
   * @param depth depth of the current generator.
   * @return llvm::Value* generated value.
   */
  llvm::Value* AllocaStructValue(llvm::IRBuilder<>& builder, llvm::StructType* type, int depth);

  /**
   * @brief Generate code to allocate a value of a struct type on the stack.
   *
   * @param builder the IR builder to use.
   * @param type the pointer type of which instances of object will be allocated.
   * @param depth depth of the current generator.
   * @return llvm::Value* allocated value.
   */
  llvm::Value* AllocaPointerType(
      llvm::IRBuilder<>& builder, llvm::PointerType* type, int depth = 0);

  /**
   * @brief Generate code to allocate a value of a struct type on the stack.
   *
   * @param builder IR builder.
   * @param type the array type of which instances of object will be allocated.
   * @param depth depth of the generator.
   * @return llvm::Value* the allocated value.
   */
  llvm::Value* AllocaArrayType(
      llvm::IRBuilder<>& builder, llvm::ArrayType* type, int depth = 0);

  /**
   * @brief Generate code to allocate a value of a struct type on the stack.
   *
   * @param builder IR builder.
   * @param type the function type of which instances of object will be allocated.
   * @param depth depth of the generator.
   * @return llvm::Value* the allocated value.
   */
  llvm::Value* AllocaFunctionType(
      llvm::IRBuilder<>& builder, llvm::FunctionType* type, int depth = 0);

  /**
   * @brief Allocate a value of the given type.
   *
   * @param builder IR builder.
   * @param type the type to allocate.
   * @param depth depth of the generator.
   * @return llvm::Value* the allocated value.
   */
  llvm::Value* AllocaValueOfType(
      llvm::IRBuilder<>& builder, llvm::Type* type, int depth = 0);

  /**
   * @brief Create a switch case inside which a call to the API function specified by the switch
   * case label is made.
   *
   * @param callee the callee of the API function call.
   * @param caller the caller of the API function call.
   * @return std::pair<llvm::ConstantInt *, llvm::BasicBlock *> a @see std::pair containing the
   * switch case label and the basic block for the switch case.
   */
  std::pair<llvm::ConstantInt *, llvm::BasicBlock *> CreateCallApiCase(
      llvm::Function* callee, llvm::Function* caller);
};

} // namespace caf

#endif
