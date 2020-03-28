#include "Targets/V8/V8Target.h"
#include "Targets/V8/V8Traits.h"

extern "C" {
  extern void** ApiFunctions;
}

namespace caf {

typename V8Traits::ApiFunctionPtrType GetApiFunction(uint32_t funcId) {
  return reinterpret_cast<typename V8Traits::ApiFunctionPtrType>(ApiFunctions);
}

}
