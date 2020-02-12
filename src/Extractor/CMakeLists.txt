add_llvm_loadable_module(CAFExtractor
    ExtractorContext.cpp
    ExtractorPass.cpp
    ExtractorUtils.cpp
    ExtractorUtils.h
    LLVMFunctionSignature.cpp
    ${CAF_INCLUDE_DIR}/Extractor/ExtractorContext.h
    ${CAF_INCLUDE_DIR}/Extractor/LLVMFunctionSignature.h

    PLUGIN_TOOL
    opt)

target_compile_definitions(CAFExtractor PUBLIC CAF_LLVM)
target_link_libraries(CAFExtractor PUBLIC CAFInfrastructure)
target_link_libraries(CAFExtractor PUBLIC CAFBasicLLVM)
