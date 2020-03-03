add_library(CAFFuzzer STATIC
    Corpus.cpp
    CorpusInitialize.h
    CorpusInitialize.cpp
    TestCaseDeserializer.cpp
    TestCaseGenerator.cpp
    TestCaseInitialize.cpp
    TestCaseInitialize.h
    TestCaseMutator.cpp
    TestCaseSerializer.cpp
    ${CAF_INCLUDE_DIR}/Fuzzer/AggregateValue.h
    ${CAF_INCLUDE_DIR}/Fuzzer/ArrayValue.h
    ${CAF_INCLUDE_DIR}/Fuzzer/BitsValue.h
    ${CAF_INCLUDE_DIR}/Fuzzer/Corpus.h
    ${CAF_INCLUDE_DIR}/Fuzzer/FunctionCall.h
    ${CAF_INCLUDE_DIR}/Fuzzer/FunctionValue.h
    ${CAF_INCLUDE_DIR}/Fuzzer/ObjectPool.h
    ${CAF_INCLUDE_DIR}/Fuzzer/PlaceholderValue.h
    ${CAF_INCLUDE_DIR}/Fuzzer/PointerValue.h
    ${CAF_INCLUDE_DIR}/Fuzzer/StructValue.h
    ${CAF_INCLUDE_DIR}/Fuzzer/TestCase.h
    ${CAF_INCLUDE_DIR}/Fuzzer/TestCaseGenerator.h
    ${CAF_INCLUDE_DIR}/Fuzzer/TestCaseDeserializer.h
    ${CAF_INCLUDE_DIR}/Fuzzer/TestCaseMutator.h
    ${CAF_INCLUDE_DIR}/Fuzzer/TestCaseSerializer.h
    ${CAF_INCLUDE_DIR}/Fuzzer/Value.h)

target_link_libraries(CAFFuzzer PUBLIC CAFInfrastructure)
target_link_libraries(CAFFuzzer PUBLIC CAFBasicPIC)

add_library(CAFMutator MODULE
    AFLExport.cpp)

target_link_libraries(CAFMutator PRIVATE CAFFuzzer)
