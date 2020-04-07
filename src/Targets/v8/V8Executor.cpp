#include "Targets/V8/V8Executor.h"

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
    v8::Local<v8::Value> function,
    typename V8Traits::ValueType receiver,
    bool isCtorCall,
    std::vector<typename V8Traits::ValueType> &args) {
  v8::EscapableHandleScope handleScope { _isolate };
  v8::TryCatch tryBlock { _isolate };

  auto func = function.As<v8::Function>();
  v8::Local<v8::Value> ret;
  if (isCtorCall) {
    ret = TryUnwrapMaybe(_isolate, func->NewInstance(_context, args.size(), args.data()));
  } else {
    ret = TryUnwrapMaybe(_isolate, func->Call(_context, receiver, args.size(), args.data()));
  }

  if (tryBlock.HasCaught()) {
    if (tryBlock.CanContinue()) {
      tryBlock.Reset();
    } else {
      // Ooops! An unusal exception has been thrown and the program should terminate.
      tryBlock.ReThrow();
    }
  }

  return handleScope.Escape(ret);
}

} // namespace caf
