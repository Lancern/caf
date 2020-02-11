#ifndef CAF_CODE_GEN_H
#define CAF_CODE_GEN_H

#include "Infrastructure/Either.h"
#include "LLVMFunctionSignature.h"

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
      _symbols(nullptr)
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
  }

  /**
   * @brief Generate CAF code stub into the context.
   *
   */
  void GenerateStub();

  /**
   * @brief Generate a global array for storing function pointers to the given list of functions.
   *
   * If some element of candidates is llvm::Function *, then the callee should leave a pointer to
   * the function at the corresponding position in the generated array; otherwise, if some element
   * of candidates is LLVMFunctionSignature, then the callee should generated a function matching
   * the signature and leave the pointer to the generated function at the corresponding position in
   * the generated array.
   *
   * @param candidates the callback function candidates.
   */
  void GenerateCallbackFunctionCandidateArray(
      const std::vector<Either<llvm::Function *, LLVMFunctionSignature>>& candidates);

private:
  llvm::Module* _module;
  const CAFSymbolTable* _symbols;
  int callbackFunctionCandidatesNum;

  llvm::CallInst* CreateNewCall(
    llvm::IRBuilder<>& builder, llvm::Type ty, llvm::Value* value
  );

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

  llvm::CallInst* CreateMemcpyCall(
    llvm::IRBuilder<>& builder, llvm::Value* dest, llvm::Value*src, llvm::Value* size);

  llvm::Function* generateEmptyFunctionWithSignature(
    LLVMFunctionSignature* funcSignature);

  llvm::Value* CreateMallocCall(
    llvm::IRBuilder<>& builder, llvm::Type* type);

  /**
   * @brief Create a call of function "inputIntTo"
   * 
   * @param builder the IR builder to use.
   * @param dest the destination of the input to
   * @return llvm::CallInst* 
   */
  llvm::CallInst* CreateInputIntToCall(
    llvm::IRBuilder<>& builder, llvm::Value* dest);
  
  /**
   * @brief Create a call of function "inputBytesTo"
   * 
   * @param builder the IR builder to use.
   * @param dest the destination of the input to
   * @param size the size of bytes to be input
   * @return llvm::CallInst* 
   */
  llvm::CallInst* CreateInputBtyesToCall(
    llvm::IRBuilder<>& builder, llvm::Value* dest, llvm::Value* size);
    
  /**
   * @brief Create a Save To Object List Call
   * 
   * @param builder 
   * @param objPtr : the object trying to save, with int64ty.
   * @return llvm::CallInst* 
   */
  llvm::CallInst* CreateSaveToObjectListCall(
    llvm::IRBuilder<>& builder, llvm::Value* objPtr);

  /**
   * @brief Create a Get From Object List Call
   * 
   * @param builder 
   * @param objIdx 
   * @return llvm::CallInst* 
   */
  llvm::CallInst* CreateGetFromObjectListCall(
    llvm::IRBuilder<>& builder, llvm::Value* objIdx);

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
  llvm::Function* CreateDispatchFunction(bool ctorDispatch = false, std::string structTypeName = std::string(""));

  /**
   * @brief Generate code to allocate a value of a struct type on the stack.
   *
   * @param builder the IR builder to use.
   * @param type the struct type of which instances of object will be allocated.
   * @param depth depth of the current generator.
   * @return llvm::Value* generated value.
   */
  llvm::Value* AllocaStructValue(llvm::IRBuilder<>& builder, llvm::StructType* type, int depth, bool init = true);

  /**
   * @brief Generate code to allocate a value of a struct type on the stack.
   *
   * @param builder the IR builder to use.
   * @param type the pointer type of which instances of object will be allocated.
   * @param depth depth of the current generator.
   * @return llvm::Value* allocated value.
   */
  llvm::Value* AllocaPointerType(
      llvm::IRBuilder<>& builder, llvm::PointerType* type, int depth = 0, bool init = true);

    llvm::Value* AllocaVectorType(
      llvm::IRBuilder<>& builder, llvm::VectorType* type, int depth = 0, bool init = true);

  /**
   * @brief Generate code to allocate a value of a struct type on the stack.
   *
   * @param builder IR builder.
   * @param type the array type of which instances of object will be allocated.
   * @param depth depth of the generator.
   * @return llvm::Value* the allocated value.
   */
  llvm::Value* AllocaArrayType(
      llvm::IRBuilder<>& builder, llvm::ArrayType* type, int depth = 0, bool init = true);

  /**
   * @brief Generate code to allocate a value of a struct type on the stack.
   *
   * @param builder IR builder.
   * @param type the function type of which instances of object will be allocated.
   * @param depth depth of the generator.
   * @return llvm::Value* the allocated value.
   */
  llvm::Value* AllocaFunctionType(
      llvm::IRBuilder<>& builder, llvm::FunctionType* type, int depth = 0, bool init = true);

  /**
   * @brief Allocate a value of the given type.
   *
   * @param builder IR builder.
   * @param type the type to allocate.
   * @param depth depth of the generator.
   * @return llvm::Value* the allocated value.
   */
  llvm::Value* AllocaValueOfType(
      llvm::IRBuilder<>& builder, llvm::Type* type, int depth = 0, bool init = true);

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
      llvm::Function* callee, llvm::Function* caller, int caseCounter, bool hasSret = false);
};

} // namespace caf

#endif
