set(V8_DIR CACHE PATH "Path to the V8 source directory")

add_library(CAFV8Target STATIC
    V8Target.cpp
    ${CAF_INCLUDE_DIR}/Targets/V8/V8Target.h)

target_include_directories(CAFV8Target PUBLIC "${V8_DIR}/include")
target_compile_options(CAFV8Target PRIVATE "-fPIC")
target_link_libraries(CAFV8Target PRIVATE CAFInfrastructure)
