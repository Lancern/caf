#include "Infrastructure/Memory.h"

#include "Targets/Common/Target.h"
#include "Targets/V8/V8Traits.h"
#include "Targets/V8/V8ArrayBuilder.h"
#include "Targets/V8/V8ValueFactory.h"
#include "Targets/V8/V8Executor.h"

#include "node.h"
#include "env.h"
#include "env-inl.h"
#include "v8.h"

#include <memory>

namespace caf {

namespace {

void RunCAF(const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto isolate = args.GetIsolate();
  v8::HandleScope handleScope { isolate };

  auto context = isolate->GetCurrentContext();
  auto callbackData = args.Data();

  auto valueFactory = caf::make_unique<V8ValueFactory>(isolate, context, callbackData);
  auto executor = caf::make_unique<V8Executor>(isolate, context, callbackData);
  Target<V8Traits> target { std::move(valueFactory), std::move(executor) };
  target.Run();
}

} // namespace <anonymous>

} // namespace caf

extern "C" NODE_MODULE_EXPORT void
NODE_MODULE_INITIALIZER(
    v8::Local<v8::Object> exports,
    v8::Local<v8::Value> module,
    v8::Local<v8::Context> context) {
  auto env = node::Environment::GetCurrent(context);
  env->SetMethod(exports, "run", &caf::RunCAF);
}
