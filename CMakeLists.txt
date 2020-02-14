cmake_minimum_required(VERSION 3.10)
project(caf)

include_directories(include dep)

set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

set(CAF_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
set(CAF_DEP_DIR "${CMAKE_SOURCE_DIR}/deps")

enable_testing()

# Configure LLVM related environments.
include(ConfigureLLVM)

# Find Clang installation.
# include(FindClang)

# Configure AFL++ related environments.
# include(ConfigureAFLpp)

add_subdirectory(deps)
add_subdirectory(src)
add_subdirectory(tests)
