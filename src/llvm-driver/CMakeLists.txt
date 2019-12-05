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
  ../../dep/json/json.hpp)

target_include_directories(CAFDriver
  PRIVATE ${LLVM_INCLUDE_DIR}
  PRIVATE ${LLVM_GEN_INCLUDE_DIR})

target_link_libraries(CAFDriver
  ${LLVM_LDFLAGS}
  ${LLVM_LIBS})
