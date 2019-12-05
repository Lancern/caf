# This CMake module finds the Clang installation and sets some related varaibles.
#
# This module will set the following variables:
#   CLANG_ROOT_DIR - root directory of the Clang installation
#   CLANG_INCLUDE_DIR - include directory of the Clang
#

if(NOT ${LLVM_BUILD_ROOT})
    message(FATAL_ERROR "LLVM_BUILD_ROOT not configured.")
endif()

set(CLANG_ROOT_DIR "${LLVM_ROOT_DIR}/clang")
if(NOT EXISTS ${CLANG_ROOT_DIR} OR NOT IS_DIRECTORY ${CLANG_ROOT_DIR})
    message(FATAL_ERROR "Could not locate clang root directory at \"${CLANG_ROOT_DIR}\"")
endif()
message(STATUS "Clang root directory located at \"${CLANG_ROOT_DIR}\"")

set(CLANG_INCLUDE_DIR "${CLANG_ROOT_DIR}/include")
set(CLANG_GEN_INCLUDE_DIR "${LLVM_BUILD_DIR}/tools/clang/include")

message(STATUS "CLANG_INCLUDE_DIR = ${CLANG_INCLUDE_DIR}")
message(STATUS "CLANG_GEN_INCLUDE_DIR = ${CLANG_GEN_INCLUDE_DIR}")
