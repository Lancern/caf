add_executable(CAFCLI
    Command.h
    CommandManager.cpp
    CommandManager.h
    Diagnostics.h
    GenerateTestCaseCommand.cpp
    main.cpp
    RegisterCommand.h)

target_link_libraries(CAFCLI PRIVATE CAFFuzzer CLI11)
set_property(TARGET CAFCLI
    PROPERTY OUTPUT_NAME "caf")
