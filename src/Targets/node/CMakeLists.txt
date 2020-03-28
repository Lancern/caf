set(NODE_DIR CACHE PATH "Path to the nodejs source diretcory")

add_custom_target(CAFNodeModule ALL
    COMMAND node-gyp configure --nodedir="${NODE_DIR}"
    COMMAND node-gyp build
    SOURCES
        NodeTarget.cpp
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
add_dependencies(CAFNodeModule CAFV8Target)

add_library(CAFNodeTarget STATIC
    NodeTarget.cpp)
target_include_directories(CAFNodeTarget PRIVATE "${NODE_DIR}/src")
target_compile_options(CAFNodeTarget PRIVATE "-fPIC" "-DNODE_WANT_INTERNALS=1")
target_link_libraries(CAFNodeTarget PRIVATE CAFV8Target CAFInfrastructure)
