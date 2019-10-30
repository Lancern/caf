add_llvm_library( CAFDriver MODULE
  CAFDriver.cpp
  CAFMeta.hpp
  CAFSymbolTable.hpp
  CAFCodeGen.hpp
  json/json.hpp

  PLUGIN_TOOL
  opt
  )
