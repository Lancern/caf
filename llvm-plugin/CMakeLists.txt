add_llvm_library( CAFDriver MODULE
  CAFDriver.cpp
  CAFMeta.hpp
  CAFSymbolTable.hpp
  json/json.hpp

  PLUGIN_TOOL
  opt
  )
