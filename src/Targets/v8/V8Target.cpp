#include "Targets/V8/V8Target.h"
#include "Targets/V8/V8Traits.h"

#define ApiFunctions callbackfunc_candidates
#define InitializeCAFStub __caf_dispatch_callbackfuncarr

extern "C" {
  extern void** ApiFunctions;

  extern void InitializeCAFStub();
}

namespace caf {

typename V8Traits::ApiFunctionPtrType GetApiFunction(uint32_t funcId) {
  static bool Initialized = false;
  if (!Initialized) {
    Initialized = true;
    InitializeCAFStub();
  }
  return reinterpret_cast<typename V8Traits::ApiFunctionPtrType>(ApiFunctions[funcId]);
}

}
