cmake_minimum_required(VERSION 3.10)
project(caf)

include_directories(include dep)

set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

# Configure LLVM related environments.
include(ConfigureLLVM)

# Find Clang installation.
# include(FindClang)

# Configure AFL++ related environments.
# include(ConfigureAFLpp)

add_subdirectory(src)
