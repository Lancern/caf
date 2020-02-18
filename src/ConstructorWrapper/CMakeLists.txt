add_library(CAFCtorWrapper MODULE
    CodeGenerator.cpp
    CodeGenerator.h
    ConstructorVisitor.cpp
    ConstructorVisitor.h
    ConstructorWrapper.cpp
    ConstructorWrapperASTConsumer.cpp
    ConstructorWrapperASTConsumer.h)

target_link_libraries(CAFCtorWrapper PUBLIC Clang)
