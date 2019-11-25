# This CMake script tries to find the LLVM installation on the local machine. If no LLVM
# installations can be found, this script reports a fatal error and stops configuring.
#
# This scripts exports the following utilities:
#   Functions:
#       llvm_config
#   Variables:
#       LLVM_CXXFLAGS
#       LLVM_LDFLAGS
#       LLVM_LIBS
#       LLVM_LIBDIR
#       LLVM_INCLUDE_DIR
#       LLVM_SYSTEM_LIBS
#

set(LLVM_BUILD_DIR "" CACHE STRING
    "Specify the build tree root of the LLVM installation")
if(NOT EXISTS "${LLVM_BUILD_DIR}" OR NOT IS_DIRECTORY "${LLVM_BUILD_DIR}")
    message(FATAL_ERROR "Could not found LLVM build tree root: \"${LLVM_BUILD_DIR}\"")
endif()
message(STATUS "Found LLVM build tree at \"${LLVM_BUILD_DIR}\"")

set(LLVM_BIN_DIR "${LLVM_BUILD_DIR}/bin")

# Find the llvm-config utility.
find_program(LLVM_CONFIG_PATH "llvm-config" "${LLVM_BIN_DIR}")
if(NOT LLVM_CONFIG_PATH)
    message(FATAL_ERROR "Could not locate llvm-config utility at \"${LLVM_BIN_DIR}\"")
endif()
message(STATUS "Found llvm-config utility at \"${LLVM_BIN_DIR}\"")

# This function invokes the llvm-config utility and stores its output into the given variable in
# the parent's scope. This function generates a fatal error if llvm-config failed.
function(llvm_config VARNAME switch)
    message(STATUS "llvm-config ${switch}")

    set(CONFIG_COMMAND "${LLVM_CONFIG_PATH}" "${switch}")
    execute_process(
        COMMAND ${CONFIG_COMMAND}
        RESULT_VARIABLE HAD_ERROR
        OUTPUT_VARIABLE CONFIG_OUTPUT
    )

    if(HAD_ERROR)
        string(REPLACE ";" " " CONFIG_COMMAND_STR "${CONFIG_COMMAND}")
        message(status ${CONFIG_COMMAND_STR})
        message(FATAL_ERROR "llvm-config failed with status ${HAD_ERROR}")
    endif()

    # Replace linebreaks with semicolon
    string(REGEX REPLACE
        "[ \t]*[\r\n]+[ \t]*" ";"
        CONFIG_OUTPUT ${CONFIG_OUTPUT})
    set(${VARNAME} ${CONFIG_OUTPUT} PARENT_SCOPE)

    unset(CONFIG_COMMAND)
endfunction(llvm_config)

llvm_config(LLVM_CXXFLAGS "--cxxflags")
llvm_config(LLVM_LDFLAGS "--ldflags")
llvm_config(LLVM_LIBS "--libs")
llvm_config(LLVM_LIB_DIR "--libdir")
llvm_config(LLVM_INCLUDE_DIR "--includedir")
llvm_config(LLVM_SYSTEM_LIBS "--system-libs")

message(STATUS "LLVM_LIB_DIR = ${LLVM_LIB_DIR}")
message(STATUS "LLVM_INCLUDE_DIR = ${LLVM_INCLUDE_DIR}")
