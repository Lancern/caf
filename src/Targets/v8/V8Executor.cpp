#include "Targets/V8/V8Executor.h"
#include "Targets/V8/V8Target.h"

#include "v8.h"

#include <utility>

namespace caf {

namespace {

template <typename T>
inline v8::Local<v8::Value> TryUnwrapMaybe(v8::Isolate* isolate, v8::MaybeLocal<T> maybe) {
  if (maybe.IsEmpty()) {
    return v8::Undefined(isolate);
  } else {
    return maybe.ToLocalChecked();
  }
}

} // namespace <anonymous>

typename V8Traits::ValueType
V8Executor::Invoke(
    uint32_t funcId,
    typename V8Traits::ValueType receiver,
    bool isCtorCall,
    std::vector<typename V8Traits::ValueType> &args) {
  auto funcPtr = reinterpret_cast<typename V8Traits::ApiFunctionPtrType>(GetApiFunction(funcId));

  v8::EscapableHandleScope handleScope { _isolate };
  auto callee = v8::Function::New(_context, funcPtr, _callbackData).ToLocalChecked();

  v8::Local<v8::Value> ret;
  if (isCtorCall) {
    ret = TryUnwrapMaybe(_isolate, callee->NewInstance(_context, args.size(), args.data()));
  } else {
    ret = TryUnwrapMaybe(_isolate, callee->Call(_context, receiver, args.size(), args.data()));
  }

  return handleScope.Escape(ret);
}

} // namespace caf
