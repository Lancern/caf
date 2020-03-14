set(CAF_EXTRACTOR_SOURCES
    ExtractorContext.cpp
    ExtractorPass.cpp
    Utils.cpp
    Utils.h
    ${CAF_INCLUDE_DIR}/Extractor/ExtractorPass.h
    ${CAF_INCLUDE_DIR}/Extractor/ExtractorContext.h)

add_library(CAFExtractor MODULE
    ${CAF_EXTRACTOR_SOURCES})

add_library(CAFExtractorLib STATIC
    ${CAF_EXTRACTOR_SOURCES})

target_compile_definitions(CAFExtractor PUBLIC CAF_LLVM)
target_link_libraries(CAFExtractor PUBLIC CAFInfrastructure)
target_link_libraries(CAFExtractor PUBLIC CAFBasic)
target_link_libraries(CAFExtractor PUBLIC LLVM)

target_compile_definitions(CAFExtractorLib PUBLIC CAF_LLVM)
target_compile_options(CAFExtractorLib PUBLIC "-fPIC")
target_link_libraries(CAFExtractorLib PUBLIC CAFInfrastructure)
target_link_libraries(CAFExtractorLib PUBLIC CAFBasic)
target_link_libraries(CAFExtractorLib PUBLIC LLVM)
