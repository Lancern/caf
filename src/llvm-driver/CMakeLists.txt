add_llvm_loadable_module( CAFDriver
  CAFDriver.cpp
  CAFMeta.hpp
  CAFSymbolTable.hpp
  CAFCodeGen.hpp
  json/json.hpp

  PLUGIN_TOOL
  opt
  )

# add_library(CAFDriver MODULE
#   CAFDriver.cpp
#   CAFCodeGen.hpp
#   CAFSymbolTable.hpp
#   ../../dep/json/json.hpp)

# target_compile_options(CAFDriver
#   PRIVATE ${LLVM_CXXFLAGS})

# target_include_directories(CAFDriver
#   PRIVATE ${LLVM_INCLUDE_DIR}
#   PRIVATE ${LLVM_GEN_INCLUDE_DIR})

# target_link_libraries(CAFDriver
#   ${LLVM_LDFLAGS}
#   ${LLVM_LIBS})

# set_target_properties(CAFDriver
#   PROPERTIES
#   ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
#   LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
#   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
