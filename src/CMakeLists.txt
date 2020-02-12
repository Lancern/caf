set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_library(NLOHMANN_JSON INTERFACE)
target_include_directories(NLOHMANN_JSON INTERFACE
    "${CAF_DEP_DIR}")
target_sources(NLOHMANN_JSON INTERFACE
    "${CAF_DEP_DIR}/json/json.hpp")

set(CAF_INFRASTRUCTURE_SOURCES
    ${CAF_INCLUDE_DIR}/Infrastructure/Casting.h
    ${CAF_INCLUDE_DIR}/Infrastructure/Either.h
    ${CAF_INCLUDE_DIR}/Infrastructure/Hash.h
    ${CAF_INCLUDE_DIR}/Infrastructure/Identity.h
    ${CAF_INCLUDE_DIR}/Infrastructure/Intrinsic.h
    ${CAF_INCLUDE_DIR}/Infrastructure/Memory.h
    ${CAF_INCLUDE_DIR}/Infrastructure/Optional.h
    ${CAF_INCLUDE_DIR}/Infrastructure/Random.h
    ${CAF_INCLUDE_DIR}/Infrastructure/TMP.h)
add_library(CAFInfrastructure INTERFACE)
target_sources(CAFInfrastructure INTERFACE
    ${CAF_INFRASTRUCTURE_SOURCES})

add_subdirectory(Basic)
# add_subdirectory(LLVMPass)
add_subdirectory(Fuzzer)
add_subdirectory(Extractor)
