add_library(CAFBasic STATIC
    CAFStore.cpp
    ${CAF_INCLUDE_DIR}/Basic/CAFStore.h
    ${CAF_INCLUDE_DIR}/Basic/Function.h
    ${CAF_INCLUDE_DIR}/Basic/FunctionSignature.h
    ${CAF_INCLUDE_DIR}/Basic/ValueKind.h)

target_link_libraries(CAFBasic
    PUBLIC NLOHMANN_JSON CAFInfrastructure)
target_compile_options(CAFBasic PRIVATE "-fPIC")
