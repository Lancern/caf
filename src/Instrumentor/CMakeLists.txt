# add_llvm_loadable_module(CAFInstrumentor
#     CAFCodeGenerator.cpp
#     CAFCodeGenerator.h
#     InstrumentorPass.cpp

#     PLUGIN_TOOL
#     opt)

add_library(CAFInstrumentor MODULE
    InstrumentorPass.cpp)

add_subdirectory(InstrumentorLib)
# add_subdirectory(common)
# add_subdirectory(nodejs)

target_compile_definitions(CAFInstrumentor PUBLIC CAF_LLVM)
target_link_libraries(CAFInstrumentor
    PUBLIC CAFInfrastructure CAFBasic CAFExtractorLib)
