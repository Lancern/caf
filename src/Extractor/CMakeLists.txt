set(CAF_EXTRACTOR_SOURCES
    ExtractorContext.cpp
    ExtractorPass.cpp
    ExtractorUtils.cpp
    ExtractorUtils.h
    LLVMFunctionSignature.cpp
    ${CAF_INCLUDE_DIR}/Extractor/ExtractorPass.h
    ${CAF_INCLUDE_DIR}/Extractor/ExtractorContext.h
    ${CAF_INCLUDE_DIR}/Extractor/LLVMFunctionSignature.h)

add_llvm_loadable_module(CAFExtractor
    ${CAF_EXTRACTOR_SOURCES}

    PLUGIN_TOOL
    opt)

add_library(CAFExtractorStatic STATIC
    ${CAF_EXTRACTOR_SOURCES})

target_compile_definitions(CAFExtractor PUBLIC CAF_LLVM)
target_link_libraries(CAFExtractor PUBLIC CAFInfrastructure)
target_link_libraries(CAFExtractor PUBLIC CAFBasicLLVM)

target_compile_definitions(CAFExtractorStatic PUBLIC CAF_LLVM)
target_compile_options(CAFExtractorStatic PUBLIC "-fPIC")
target_link_libraries(CAFExtractorStatic PUBLIC CAFInfrastructure)
target_link_libraries(CAFExtractorStatic PUBLIC CAFBasicLLVM)
