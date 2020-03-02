add_subdirectory(googletest)
add_subdirectory(json)
add_subdirectory(CLI11)

set(gtest_include_dir "${CMAKE_CURRENT_SOURCE_DIR}/googletest/googletest/include" PARENT_SCOPE)
