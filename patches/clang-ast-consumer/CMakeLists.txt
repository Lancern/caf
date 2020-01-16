add_library(CAFClangPlugin SHARED
    CAFConstructorWrapper.cpp)

target_compile_options(CAFClangPlugin
    PRIVATE ${LLVM_CXXFLAGS})

target_include_directories(CAFClangPlugin
    PRIVATE ${LLVM_INCLUDE_DIR}
    PRIVATE ${LLVM_GEN_INCLUDE_DIR}
    PRIVATE ${CLANG_INCLUDE_DIR}
    PRIVATE ${CLANG_GEN_INCLUDE_DIR})

target_link_libraries(CAFClangPlugin
    ${LLVM_LDFLAGS}
    ${LLVM_LIBS}
    "-L${CLANG_LIB_DIR}"
    ${CLANG_LIBS})

set_target_properties(CAFClangPlugin
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
