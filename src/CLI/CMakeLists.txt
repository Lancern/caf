add_executable(CAFCLI
    Command.h
    CommandManager.cpp
    CommandManager.h
    Diagnostics.h
    FuzzCommand.cpp
    GenerateTestCaseCommand.cpp
    main.cpp
    Printer.cpp
    Printer.h
    RegisterCommand.h
    ShowCommand.cpp
    StatCommand.cpp
    SynthesisCommand.cpp
    TestCaseDumper.cpp
    TestCaseDumper.h)

target_link_libraries(CAFCLI PRIVATE CAFFuzzer CLI11)
set_property(TARGET CAFCLI
    PROPERTY OUTPUT_NAME "caf")
