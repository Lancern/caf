cmake_minimum_required(VERSION 3.10)
project(caf)

include_directories(include dep)

# Find LLVM installation.
include(FindLLVM.cmake)

# Find AFL++ installation.
include(FindAFLPlusPlus.cmake)

add_subdirectory(src)
