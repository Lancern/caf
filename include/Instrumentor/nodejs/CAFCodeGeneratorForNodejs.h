#ifndef CAF_CODE_GENERATOR_FOR_NODEJS
#define CAF_CODE_GENERATOR_FOR_NODEJS

#include "llvm/IR/IRBuilder.h"

namespace llvm {
class Module;
class CallInst;
} // namespace llvm

namespace caf {

class ExtractorContext;

/**
 * @brief Provide logic for generate CAF code stub.
 *
 */
class CAFCodeGeneratorForNodejs {
public:
  /**
   * @brief Construct a new CAFCodeGeneratorForNodejs object.
   *
   */
  explicit CAFCodeGeneratorForNodejs()
    : _module(nullptr),
      _extraction(nullptr)
  { }

  /**
   * @brief Set the context of the code generator.
   *
   * @param module the module into which the code stub will be generated.
   * @param extraction the extracted data by the extractor pass.
   */
  void SetContext(llvm::Module& module, const ExtractorContext& extraction) {
    _module = &module;
    _extraction = &extraction;
  }

  /**
   * @brief Generate CAF code stub into the context.
   *
   */
  void GenerateStub();

private:
  llvm::Module* _module;
  const ExtractorContext* _extraction;
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

  llvm::CallInst* CreateInputDoubleToCall(
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
   * @brief Create a Get Random Bytes Call 
   * 
   * @param builder 
   * @param bytesSize 
   * @return llvm::CallInst* 
   */
  llvm::CallInst* CreateGetRandomBytesCall(
    llvm::IRBuilder<>& builder, int bytesSize);

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
  llvm::Function* CreateDispatchFunctionForApi();

  /**
   * @brief Create a Dispatch Function Decel For Types
   * 
   * @param type 
   */
  void CreateDispatchFunctionDecelForTypes(llvm::Type* type);

  /**
   * @brief Create a Dispatch Function Decel For Ctors
   * 
   * @param ctorTypeId 
   */
  void CreateDispatchFunctionDecelForCtors(uint64_t ctorTypeId);
  /**
   * @brief Create a Dispatch Function For Types 
   * 
   * @param type 
   * @return llvm::Function* : the dispath function to create a value for the type
   */
  llvm::Function* CreateDispatchFunctionForTypes(llvm::Type* type);

  /**
   * @brief Generate the dispatch function for invoke ctors in the module.
   * 
   * @param ctorTypeId 
   * @return llvm::Function* 
   */
  llvm::Function* CreateDispatchFunctionForCtors(uint64_t ctorTypeId);
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
  

  llvm::Value* MallocUndefinedType(llvm::IRBuilder<>& builder);

  llvm::Value* MallocNullType(llvm::IRBuilder<>& builder);

  llvm::Value* MallocStringType(llvm::IRBuilder<>& builder);

  llvm::Value* MallocIntegerType(llvm::IRBuilder<>& builder);

  llvm::Value* MallocBooleanType(llvm::IRBuilder<>& builder);

  llvm::Value* MallocFloatingPointerType(llvm::IRBuilder<>& builder);

  llvm::Value* MallocFunctionType(llvm::IRBuilder<>& builder);

  llvm::Value* MallocArrayType(llvm::IRBuilder<>& builder);

  llvm::Value* MallocPlaceholderType(llvm::IRBuilder<>& builder);

  /**
   * @brief Create a Dispatch to Malloc Value Of Type 
   * 
   * @return llvm::Function* 
   */
  llvm::Function* CreateDispatchMallocValueOfType();

  
  void GenerateCallbackFunctionCandidateArray();

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
      llvm::Function* callee, llvm::Function* caller, int caseCounter);
};

} // namespace caf

#endif
