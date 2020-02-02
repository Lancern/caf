set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_library(NLOHMANN_JSON INTERFACE)
target_include_directories(NLOHMANN_JSON INTERFACE
    "${CAF_DEP_DIR}")
target_sources(NLOHMANN_JSON INTERFACE
    "${CAF_DEP_DIR}/json/json.hpp")

add_subdirectory(Basic)
add_subdirectory(LLVMPass)
add_subdirectory(Fuzzer)
