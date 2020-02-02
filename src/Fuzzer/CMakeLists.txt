add_library(CAFMutator MODULE
    AFLExport.cpp
    Corpus.cpp
    TestCaseMutator.cpp
    ValueGenerator.cpp
    ${CAF_INCLUDE_DIR}/Fuzzer/ArrayValue.h
    ${CAF_INCLUDE_DIR}/Fuzzer/BitsValue.h
    ${CAF_INCLUDE_DIR}/Fuzzer/Corpus.h
    ${CAF_INCLUDE_DIR}/Fuzzer/FunctionCall.h
    ${CAF_INCLUDE_DIR}/Fuzzer/ObjectPool.h
    ${CAF_INCLUDE_DIR}/Fuzzer/PointerValue.h
    ${CAF_INCLUDE_DIR}/Fuzzer/StructValue.h
    ${CAF_INCLUDE_DIR}/Fuzzer/TestCase.h
    ${CAF_INCLUDE_DIR}/Fuzzer/TestCaseMutator.h
    ${CAF_INCLUDE_DIR}/Fuzzer/TestCaseSerializer.h
    ${CAF_INCLUDE_DIR}/Fuzzer/Value.h
    ${CAF_INCLUDE_DIR}/Fuzzer/ValueGenerator.h)

target_link_libraries(CAFMutator PRIVATE CAFBasicPIC)
