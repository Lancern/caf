add_executable(CAFClang
    main.cpp)
add_executable(CAFClangPP
    main.cpp)

target_compile_definitions(CAFClangPP
    PRIVATE "CAF_CLANG_CXX")

set_property(TARGET CAFClang
    PROPERTY OUTPUT_NAME "caf-clang")
set_property(TARGET CAFClangPP
    PROPERTY OUTPUT_NAME "caf-clang++")
