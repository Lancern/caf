add_llvm_loadable_module(CAFPass
    ABI.cpp
    ABI.h
    CAFDriver.cpp
    CodeGen.cpp
    CodeGen.h
    FunctionSignatureGrouper.cpp
    FunctionSignatureGrouper.h
    LLVMFunctionSignature.cpp
    LLVMFunctionSignature.h
    SymbolTable.cpp
    SymbolTable.h

    PLUGIN_TOOL
    opt)

target_link_libraries(CAFPass
    PUBLIC CAFBasicLLVM)
