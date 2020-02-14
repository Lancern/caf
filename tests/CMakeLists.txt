add_executable(CAFTests
    main.cpp
    Infrastructure/Optional.cpp)

# target_include_directories(CAFTests PRIVATE ${gtest_include_dir})
target_link_libraries(CAFTests PRIVATE gtest CAFInfrastructure)

add_test(NAME CAFTests COMMAND CAFTests)
