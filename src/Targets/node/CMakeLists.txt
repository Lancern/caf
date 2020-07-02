set(NODE_DIR CACHE PATH "Path to the nodejs source diretcory")

if(CMAKE_BUILD_TYPE EQUAL "Release")
    add_custom_target(CAFNodeModule
        COMMAND node-gyp rebuild --nodedir="${NODE_DIR}"
        COMMAND cp ./build/Release/caf.node "${CMAKE_BINARY_DIR}/lib"
        SOURCES
            NodeTarget.cpp
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
else()
    add_custom_target(CAFNodeModule
        COMMAND node-gyp rebuild --debug --nodedir="${NODE_DIR}"
        COMMAND cp ./build/Debug/caf.node "${CMAKE_BINARY_DIR}/lib"
        SOURCES
            NodeTarget.cpp
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
add_dependencies(CAFNodeModule CAFV8Target)

