add_llvm_loadable_module(CAFDriver
  CAFDriver.cpp
  CAFMeta.hpp
  CAFSymbolTable.hpp
  CAFCodeGen.hpp
  json/json.hpp

  PLUGIN_TOOL
  opt
  )

set_target_properties(CAFDriver
  PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
