set(CAF_BASIC_SOURCES
    CAFStore.cpp
    JsonSerializer.cpp
    JsonDeserializer.cpp
    ${CAF_INCLUDE_DIR}/Basic/Identity.h
    ${CAF_INCLUDE_DIR}/Basic/Type.h
    ${CAF_INCLUDE_DIR}/Basic/NamedType.h
    ${CAF_INCLUDE_DIR}/Basic/BitsType.h
    ${CAF_INCLUDE_DIR}/Basic/PointerType.h
    ${CAF_INCLUDE_DIR}/Basic/ArrayType.h
    ${CAF_INCLUDE_DIR}/Basic/StructType.h
    ${CAF_INCLUDE_DIR}/Basic/Function.h
    ${CAF_INCLUDE_DIR}/Basic/Constructor.h
    ${CAF_INCLUDE_DIR}/Basic/CAFStore.h
    ${CAF_INCLUDE_DIR}/Basic/JsonSerializer.h
    ${CAF_INCLUDE_DIR}/Basic/JsonDeserializer.h
    ${CAF_INCLUDE_DIR}/Infrastructure/Casting.h
    ${CAF_INCLUDE_DIR}/Infrastructure/Intrinsic.h
    ${CAF_INCLUDE_DIR}/Infrastructure/Memory.h
    ${CAF_INCLUDE_DIR}/Infrastructure/TMP.h)

add_library(CAFBasic STATIC
    ${CAF_BASIC_SOURCES})
add_library(CAFBasicLLVM STATIC
    ${CAF_BASIC_SOURCES})

target_compile_definitions(CAFBasicLLVM
    PUBLIC CAF_LLVM)

target_link_libraries(CAFBasic PUBLIC NLOHMANN_JSON)
target_link_libraries(CAFBasicLLVM PUBLIC NLOHMANN_JSON)

# CAFBasicLLVM should be compiled as PIC since it will be linked against LLVM loadable modules.
target_compile_options(CAFBasicLLVM PRIVATE "-fPIC")
