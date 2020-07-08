add_library(CAFProber STATIC
    NodeExecutor.cpp
    ${CAF_INCLUDE_DIR}/Prober/NodeExecutor.h
    ${CAF_INCLUDE_DIR}/Prober/ProbeExecutor.h)

target_link_libraries(CAFProber PUBLIC CAFBasic CAFFuzzer)
