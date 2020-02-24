add_library(CAFCtorWrapper MODULE
    CtorWrapper.cpp
    CtorWrapperASTConsumer.cpp
    CtorWrapperASTConsumer.h
    CtorWrapperCodeGen.cpp
    CtorWrapperCodeGen.h)

target_link_libraries(CAFCtorWrapper PUBLIC Clang)
