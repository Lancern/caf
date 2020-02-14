# add_llvm_loadable_module(CAFInstrumentor
#     CAFCodeGenerator.cpp
#     CAFCodeGenerator.h
#     InstrumentorPass.cpp

#     PLUGIN_TOOL
#     opt)

add_library(CAFInstrumentor MODULE
    CAFCodeGenerator.cpp
    CAFCodeGenerator.h
    InstrumentorPass.cpp)

target_compile_definitions(CAFInstrumentor PUBLIC CAF_LLVM)
target_link_libraries(CAFInstrumentor PUBLIC CAFInfrastructure)
target_link_libraries(CAFInstrumentor PUBLIC CAFBasicLLVM)
target_link_libraries(CAFInstrumentor PRIVATE CAFExtractorStatic)
target_link_libraries(CAFInstrumentor PUBLIC LLVM)
