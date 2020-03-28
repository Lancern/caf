#include "Targets/V8/V8Target.h"
#include "Targets/V8/V8Traits.h"

#define API_FUNCS callbackfunc_candidates

extern "C" {
  extern void** API_FUNCS;
}

namespace caf {

typename V8Traits::ApiFunctionPtrType GetApiFunction(uint32_t funcId) {
  return reinterpret_cast<typename V8Traits::ApiFunctionPtrType>(API_FUNCS[funcId]);
}

}
