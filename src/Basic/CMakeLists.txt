set(CAF_BASIC_SOURCES
    CAFStore.cpp
    JsonSerializer.cpp
    JsonDeserializer.cpp
    ${CAF_INCLUDE_DIR}/Basic/Type.h
    ${CAF_INCLUDE_DIR}/Basic/NamedType.h
    ${CAF_INCLUDE_DIR}/Basic/BitsType.h
    ${CAF_INCLUDE_DIR}/Basic/PointerType.h
    ${CAF_INCLUDE_DIR}/Basic/ArrayType.h
    ${CAF_INCLUDE_DIR}/Basic/StructType.h
    ${CAF_INCLUDE_DIR}/Basic/FunctionType.h
    ${CAF_INCLUDE_DIR}/Basic/Function.h
    ${CAF_INCLUDE_DIR}/Basic/Constructor.h
    ${CAF_INCLUDE_DIR}/Basic/CAFStore.h
    ${CAF_INCLUDE_DIR}/Basic/JsonSerializer.h
    ${CAF_INCLUDE_DIR}/Basic/JsonDeserializer.h)

add_library(CAFBasic STATIC
    ${CAF_BASIC_SOURCES})
add_library(CAFBasicLLVM STATIC
    ${CAF_BASIC_SOURCES})
add_library(CAFBasicPIC STATIC
    ${CAF_BASIC_SOURCES})

target_link_libraries(CAFBasicLLVM  PUBLIC LLVM)

target_compile_options(CAFBasicPIC  PUBLIC "-fPIC")

target_link_libraries(CAFBasic      PUBLIC NLOHMANN_JSON)
target_link_libraries(CAFBasicLLVM  PUBLIC NLOHMANN_JSON)
target_link_libraries(CAFBasicPIC   PUBLIC NLOHMANN_JSON)

target_link_libraries(CAFBasic      PUBLIC CAFInfrastructure)
target_link_libraries(CAFBasicLLVM  PUBLIC CAFInfrastructure)
target_link_libraries(CAFBasicPIC   PUBLIC CAFInfrastructure)

