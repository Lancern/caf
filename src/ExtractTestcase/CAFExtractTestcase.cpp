
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


namespace {
/**
 * @brief Implement an LLVM function pass to extract structtype testcase.
 *
 */
class CAFExtractTestcase : public llvm::FunctionPass {
public:
  /**
   * @brief This field is used as an alternative machenism to RTTI in LLVM.
   *
   */
  static char ID;

  /**
   * @brief Construct a new CAFDriver object.
   */
  explicit CAFExtractTestcase()
    : FunctionPass(ID)
  { }

  /**
   * @brief This function is the entry point of the function pass.
   *
   * @param Function The LLVM Function to work on.
   * @return true
   * @return false
   */
  bool runOnFunction(llvm::Function &Function) override {
    
    // llvm::errs() << Function.getName().str() << "\n";
    for(auto &BB: Function) {
        for(auto &I: BB) {
            if(llvm::isa<llvm::CallInst>(I) || llvm::isa<llvm::InvokeInst>(I)) {
               for(auto &operand: I.operands()) {
                   if(llvm::isa<llvm::Function>(operand)) {
                       auto callee = llvm::cast<llvm::Function>(operand);
                       if(callee->hasFnAttribute(llvm::Attribute::CafCxxCtor)) { 
                           extractTestcaseFrom(I.getOperand(0)); // this*
                           break;
                       }
                   }
               }
            }
        }
    }

    return false;
  }

private:

    enum CafType {
        IntegerOrFloatingpointTy = 0,
        PointerTy,
        ArrayOrVectorTy,
        StructTy,
        ObjectReuseTy,
        FunctionTy
    };

    std::vector<int8_t> extractTestcaseFrom(llvm::Value* value) {
        llvm::Type* type = value->getType();
        if(type->isIntegerTy() || type->isFloatingPointTy()) {
            
        } else if(type->isPointerTy()) {


        } else if(type->isVectorTy()) {

        } else if(type->isArrayTy()) {

        } else if(type->isStructTy()) {

        } else if(type->isFunctionTy()) {

        }
        
        // llvm::Function* ctor;
        // std::vector<llvm::Value*> params = { };
        // for(auto &operand: callCtor.operands()) {
        //     // llvm::errs() << "\n operand: \n";
        //     // operand->dump();
        //     if(llvm::isa<llvm::Function>(operand)) {
        //         auto fn = llvm::cast<llvm::Function>(operand);
        //         if(fn->hasFnAttribute(llvm::Attribute::CafCxxCtor)) { 
        //             ctor = fn;
        //             break;
        //         }
        //     } else {
        //         params.push_back(operand);
        //     }
        // }
        // auto inputKind = CafType::StructTy; // struct type
        // auto ctorId = getCtorId(ctor);
        
        // std::vector<char>testcase;
        // // testcase.push_back(inputKind);
        // // testcase.push_back(ctorId);
        // for(auto param: params) {
        //     extractTestcaseFrom(param);
        // }
        

    }

    int getCtorId(llvm::Function* ctor) {
        // TODO: Give a ctor, return the ctorId.
        return 0;
    }

    int getFunctionId(llvm::Function* fn) {
        // TODO: Give a function, return the functionId.
        return 0;
    }
  
}; // class CAFDriver

}  // namespace <anonymous>

char CAFExtractTestcase::ID = 0;
static llvm::RegisterPass<CAFExtractTestcase> X(
    "caf_extract_testcase", "CAFExtractTestcase Pass",
    false, // Only looks at CFG
    false); // Analysis Pass

static llvm::RegisterStandardPasses Y(
    llvm::PassManagerBuilder::EP_EarlyAsPossible,
    [] (const llvm::PassManagerBuilder &, llvm::legacy::PassManagerBase &m) {
        m.add(new CAFExtractTestcase());
    });
