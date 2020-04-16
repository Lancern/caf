add_library(CAFFuzzer STATIC
    BrowserSynthesisBuilder.cpp
    JavaScriptSynthesisBuilder.cpp
    NodejsSynthesisBuilder.cpp
    ObjectPool.cpp
    SynthesisBuilder.cpp
    TestCaseDeserializer.cpp
    TestCaseGenerator.cpp
    TestCaseMutator.cpp
    TestCaseSerializer.cpp
    TestCaseSynthesiser.cpp
    ${CAF_INCLUDE_DIR}/Fuzzer/FunctionCall.h
    ${CAF_INCLUDE_DIR}/Fuzzer/JavaScriptSynthesisBuilder.h
    ${CAF_INCLUDE_DIR}/Fuzzer/NodejsSynthesisBuilder.h
    ${CAF_INCLUDE_DIR}/Fuzzer/ObjectPool.h
    ${CAF_INCLUDE_DIR}/Fuzzer/SynthesisBuilder.h
    ${CAF_INCLUDE_DIR}/Fuzzer/TestCase.h
    ${CAF_INCLUDE_DIR}/Fuzzer/TestCaseDeserializer.h
    ${CAF_INCLUDE_DIR}/Fuzzer/TestCaseGenerator.h
    ${CAF_INCLUDE_DIR}/Fuzzer/TestCaseMutator.h
    ${CAF_INCLUDE_DIR}/Fuzzer/TestCaseSerializer.h
    ${CAF_INCLUDE_DIR}/Fuzzer/TestCaseSynthesiser.h
    ${CAF_INCLUDE_DIR}/Fuzzer/Value.h)

target_link_libraries(CAFFuzzer PUBLIC CAFInfrastructure CAFBasic)
target_compile_options(CAFFuzzer PRIVATE "-fPIC")

add_library(CAFMutator MODULE
    AFLExport.cpp)

target_link_libraries(CAFMutator PRIVATE CAFFuzzer)
