set(NODE_DIR CACHE PATH "Path to the nodejs source diretcory")
add_custom_target(CAFNodeTarget ALL
    COMMAND npm i
    COMMAND node-gyp configure --nodedir="${NODE_DIR}"
    COMMAND node-gyp build
    SOURCES
        src/NodeTarget.cpp
        src/NodeTarget.h
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
add_dependencies(CAFNodeTarget CAFV8Target)
