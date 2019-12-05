add_library(CAFClangPlugin SHARED
    CAFConstructorWrapper.cpp)

target_include_directories(CAFClangPlugin
    PRIVATE ${LLVM_INCLUDE_DIR}
    PRIVATE ${LLVM_GEN_INCLUDE_DIR}
    PRIVATE ${CLANG_INCLUDE_DIR}
    PRIVATE ${CLANG_GEN_INCLUDE_DIR})

target_link_libraries(CAFClangPlugin
    ${LLVM_LDFLAGS}
    ${LLVM_LIBS}
    clangFrontend)
