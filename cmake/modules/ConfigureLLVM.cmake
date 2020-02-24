# This CMake scripts tries to locate and configure a LLVM installation.

set(LLVM_BUILD_DIR "" CACHE PATH "Paths to the root of the build tree of LLVM")
set(LLVM_BIN_DIR "${LLVM_BUILD_DIR}/bin")

function(get_llvm_config var_name config)
    execute_process(
        COMMAND "${LLVM_BIN_DIR}/llvm-config" "--${config}"
        OUTPUT_VARIABLE value)
    string(REGEX REPLACE "[ \t\r]+" ";" value "${value}")
    string(REGEX REPLACE "\n" "" value "${value}")
    set(${var_name} "${value}" PARENT_SCOPE)
endfunction(get_llvm_config)

get_llvm_config(LLVM_SOURCE_DIR "src-root")
get_llvm_config(LLVM_INCLUDE_DIR "includedir")
get_llvm_config(LLVM_CXXFLAGS "cxxflags")
get_llvm_config(LLVM_LDFLAGS "ldflags")
get_llvm_config(LLVM_LIBS "libs")

set(CLANG_SOURCE_DIR "${LLVM_SOURCE_DIR}/tools/clang")
set(CLANG_INCLUDE_DIR
    "${CLANG_SOURCE_DIR}/include"
    "${LLVM_BUILD_DIR}/tools/clang/include")

add_library(LLVM INTERFACE)
target_include_directories(LLVM INTERFACE ${LLVM_INCLUDE_DIR})
target_compile_definitions(LLVM INTERFACE "CAF_LLVM")
target_compile_options(LLVM INTERFACE ${LLVM_CXXFLAGS} ${LLVM_LDFLAGS} ${LLVM_LIBS})

add_library(Clang INTERFACE)
target_include_directories(Clang INTERFACE ${CLANG_INCLUDE_DIR})
target_link_libraries(Clang INTERFACE LLVM)
