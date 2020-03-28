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

#include <csignal>
#include <cstdlib>
#include <cstdio>
#include <memory>

namespace caf {

namespace {

void AbortHandler(int) {
  std::fprintf(stderr, "aborted.\n");
  std::exit(0);
}

void SetupAbortHandler() {
  if (std::signal(SIGABRT, &AbortHandler) == SIG_ERR) {
    std::fprintf(stderr, "Failed to setup abort handler.\n");
    std::exit(1);
  }
}

v8::Local<v8::Object> CreateCallbackData(
    v8::Isolate* isolate, v8::Local<v8::Context> context, node::Environment* env) {
  v8::EscapableHandleScope handleScope { isolate };
  v8::Local<v8::FunctionTemplate> ctor = v8::FunctionTemplate::New(isolate);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  v8::Local<v8::Object> ret =
      ctor->GetFunction(context).ToLocalChecked()
          ->NewInstance(context).ToLocalChecked();
  ret->SetAlignedPointerInInternalField(0, env);
  return handleScope.Escape(ret);
}

void RunCAF(const v8::FunctionCallbackInfo<v8::Value>& args) {
  SetupAbortHandler();

  auto isolate = args.GetIsolate();
  v8::HandleScope handleScope { isolate };

  auto context = isolate->GetCurrentContext();
  auto env = node::Environment::GetCurrent(context);
  auto callbackData = CreateCallbackData(isolate, context, env);

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
