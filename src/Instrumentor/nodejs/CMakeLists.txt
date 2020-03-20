add_library(CAFInstrumentorForNodejs STATIC
    CAFCodeGenerator.cpp)

target_compile_definitions(CAFInstrumentorForNodejs PUBLIC CAF_LLVM)
target_link_libraries(CAFInstrumentorForNodejs
    PUBLIC CAFInfrastructure CAFBasic CAFExtractorLib)