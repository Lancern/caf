# add_llvm_library( CAFDriver MODULE
#   CAFDriver.cpp
#   CAFMeta.hpp
#   CAFSymbolTable.hpp
#   CAFCodeGen.hpp
#   json.hpp

#   PLUGIN_TOOL
#   opt
#   )

add_library(CAFDriver MODULE
  CAFDriver.cpp
  CAFCodeGen.hpp
  CAFSymbolTable.hpp
  ../lib/json/json.hpp)
target_link_libraries(CAFDriver
  ${LLVM_LDFLAGS}
  ${LLVM_LIBS})
