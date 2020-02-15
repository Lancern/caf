add_llvm_loadable_module(CAFExtractTestcasePass
    CAFExtractTestcase.cpp

    PLUGIN_TOOL
    opt)

target_link_libraries(CAFExtractTestcasePass PUBLIC CAFInfrastructure)
target_link_libraries(CAFExtractTestcasePass PUBLIC CAFBasicLLVM)

# add_subdirectory(libCAF)
