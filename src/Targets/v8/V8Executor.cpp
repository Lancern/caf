#include "Targets/V8/V8Executor.h"
#include "Targets/V8/V8Target.h"

#include "v8.h"

namespace caf {

typename V8Traits::ValueType
V8Executor::Invoke(
    uint32_t funcId,
    typename V8Traits::ValueType receiver,
    std::vector<typename V8Traits::ValueType> &args) {
  auto funcPtr = reinterpret_cast<typename V8Traits::ApiFunctionPtrType>(GetApiFunction(funcId));

  v8::EscapableHandleScope handleScope { _isolate };
  auto callee = v8::Function::New(_context, funcPtr, _callbackData).ToLocalChecked();
  auto ret = callee->Call(_context, receiver, args.size(), args.data());

  if (ret.IsEmpty()) {
    return handleScope.Escape(v8::Undefined(_isolate));
  } else {
    return handleScope.Escape(ret.ToLocalChecked());
  }
}

} // namespace caf
