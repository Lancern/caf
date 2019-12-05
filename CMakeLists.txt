cmake_minimum_required(VERSION 3.10)
project(caf)

include_directories(include dep)

set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

# Find LLVM installation.
include(FindLLVM)

# Find Clang installation.
include(FindClang)

# Find AFL++ installation.
include(FindAFLPlusPlus)

add_subdirectory(src)
