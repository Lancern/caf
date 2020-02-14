# This CMake scripts tries to locate and configure a LLVM installation.

set(LLVM_BUILD_DIR "" CACHE PATH "Paths to the root of the build tree of LLVM")
set(LLVM_BIN_DIR "${LLVM_BUILD_DIR}/bin")

function(get_llvm_config var_name config)
    execute_process(
        COMMAND "${LLVM_BIN_DIR}/llvm-config" "--${config}"
        OUTPUT_VARIABLE value)
    string(REGEX REPLACE "[ \n\t\r]+" ";" value "${value}")
    set(${var_name} "${value}" PARENT_SCOPE)
endfunction(get_llvm_config)

get_llvm_config(LLVM_INCLUDE_DIR "includedir")
get_llvm_config(LLVM_CXXFLAGS "cxxflags")
get_llvm_config(LLVM_LDFLAGS "ldflags")
get_llvm_config(LLVM_LIBS "libs")

add_library(LLVM INTERFACE)
target_include_directories(LLVM INTERFACE ${LLVM_INCLUDE_DIR})
target_compile_definitions(LLVM INTERFACE "CAF_LLVM")
target_compile_options(LLVM INTERFACE ${LLVM_CXXFLAGS} ${LLVM_LDFLAGS} ${LLVM_LIBS})
