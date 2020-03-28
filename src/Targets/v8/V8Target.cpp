#include "Targets/V8/V8Target.h"
#include "Targets/V8/V8Traits.h"

#include <cassert>

extern "C" {
  extern void* __caf_GetApiFunction(uint32_t funcId);
}

namespace caf {

typename V8Traits::ApiFunctionPtrType GetApiFunction(uint32_t funcId) {
  auto ptr = reinterpret_cast<typename V8Traits::ApiFunctionPtrType>(__caf_GetApiFunction(funcId));
  assert(ptr && "Invalid function ID.");
  return ptr;
}

}
