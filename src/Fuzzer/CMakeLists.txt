add_library(CAFMutator MODULE
    AFLExport.cpp
    Corpus.cpp
    CorpusInitialize.h
    CorpusInitialize.cpp
    TestCaseMutator.cpp
    TestCaseSerializer.cpp
    ValueGenerator.cpp
    ${CAF_INCLUDE_DIR}/Fuzzer/ArrayValue.h
    ${CAF_INCLUDE_DIR}/Fuzzer/BitsValue.h
    ${CAF_INCLUDE_DIR}/Fuzzer/Corpus.h
    ${CAF_INCLUDE_DIR}/Fuzzer/FunctionCall.h
    ${CAF_INCLUDE_DIR}/Fuzzer/FunctionPointerValue.h
    ${CAF_INCLUDE_DIR}/Fuzzer/ObjectPool.h
    ${CAF_INCLUDE_DIR}/Fuzzer/PlaceholderValue.h
    ${CAF_INCLUDE_DIR}/Fuzzer/PointerValue.h
    ${CAF_INCLUDE_DIR}/Fuzzer/StructValue.h
    ${CAF_INCLUDE_DIR}/Fuzzer/TestCase.h
    ${CAF_INCLUDE_DIR}/Fuzzer/TestCaseMutator.h
    ${CAF_INCLUDE_DIR}/Fuzzer/TestCaseSerializer.h
    ${CAF_INCLUDE_DIR}/Fuzzer/Value.h
    ${CAF_INCLUDE_DIR}/Fuzzer/ValueGenerator.h)

target_link_libraries(CAFMutator PRIVATE CAFInfrastructure)
target_link_libraries(CAFMutator PRIVATE CAFBasicPIC)
