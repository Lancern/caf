# This CMake scripts tries to locate and configure a LLVM 7.1.0 installation.

set(LLVM_BUILD_DIR "" CACHE PATH "Paths to the root of the build tree of LLVM")

find_package(LLVM 7.1.0 EXACT REQUIRED CONFIG
    PATHS "${LLVM_BUILD_DIR}")

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)
