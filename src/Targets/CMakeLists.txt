# add_library(CAFCommonTarget INTERFACE)
# target_sources(CAFCommonTarget INTERFACE
#     ${CAF_INCLUDE_DIR}/Targets/Common/AbstractExecutor.h
#     ${CAF_INCLUDE_DIR}/Targets/Common/Diagnostics.h
#     ${CAF_INCLUDE_DIR}/Targets/Common/FunctionDatabase.h
#     ${CAF_INCLUDE_DIR}/Targets/Common/PropertyResolver.h
#     ${CAF_INCLUDE_DIR}/Targets/Common/Target.h
#     ${CAF_INCLUDE_DIR}/Targets/Common/TestCaseParser.h
#     ${CAF_INCLUDE_DIR}/Targets/Common/ValueFactory.h)
# target_link_libraries(CAFCommonTarget INTERFACE CAFInfrastructure CAFBasic)

# add_subdirectory(v8)
add_subdirectory(node)
