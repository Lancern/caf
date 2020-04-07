#include "Infrastructure/Memory.h"
#include "Basic/CAFStore.h"
#include "Targets/Common/Diagnostics.h"
#include "Targets/Common/Target.h"
#include "Targets/V8/V8Traits.h"
#include "Targets/V8/V8ArrayBuilder.h"
#include "Targets/V8/V8ValueFactory.h"
#include "Targets/V8/V8Executor.h"
#include "Targets/V8/V8PropertyResolver.h"

#include "json/json.hpp"

#include "node.h"
#include "env.h"
#include "env-inl.h"
#include "v8.h"

#include <cassert>
#include <csignal>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <memory>

#ifdef CAF_ENABLE_AFL_PERSIST

extern "C" {
  int __afl_persistent_loop(unsigned loops);
}

#define CAF_AFL_PERSIST_LOOPS 100

#endif

#ifdef CAF_ENABLE_AFL_DEFER

extern "C" {
  void __afl_manual_init();
}

#endif

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

std::unique_ptr<CAFStore> GetCAFStore() {
  auto filePath = std::getenv("CAF_STORE");
  if (!filePath) {
    PRINT_ERR_AND_EXIT("CAF_STORE not set.\n");
    return nullptr;
  }

  std::ifstream file { filePath };
  if (file.fail()) {
    PRINT_ERR_AND_EXIT("failed to load CAF metadata store.\n");
    return nullptr;
  }

  nlohmann::json json;
  file >> json;

  return caf::make_unique<CAFStore>(json);
}

bool IsInAFL() {
  return getenv("__AFL_SHM_ID") || getenv("__AFL_CMPLOG_SHM_ID");
}

void RunCAF(const v8::FunctionCallbackInfo<v8::Value>& args) {
  SetupAbortHandler();

  auto isolate = args.GetIsolate();
  v8::HandleScope handleScope { isolate };

  auto context = isolate->GetCurrentContext();
  auto env = node::Environment::GetCurrent(context);
  auto callbackData = CreateCallbackData(isolate, context, env);

  if (args.Length() < 1) {
    PRINT_ERR_AND_EXIT("caf.run should be given at least 1 argument.\n");
  }

  auto valueFactory = caf::make_unique<V8ValueFactory>(isolate, context, callbackData);
  auto executor = caf::make_unique<V8Executor>(isolate, context, callbackData);
  auto resolver = caf::make_unique<V8PropertyResolver>(context);
  auto global = args[0];
  Target<V8Traits> target {
      std::move(valueFactory), std::move(executor), std::move(resolver), global };

  auto store = GetCAFStore();
  target.functions().Populate(*store);

#ifdef CAF_ENABLE_AFL_DEFER
  __afl_manual_init();
#endif

#ifdef CAF_ENABLE_AFL_PERSIST
  while (__afl_persistent_loop(CAF_AFL_PERSIST_LOOPS)) {
#endif

  target.Run();

  if (IsInAFL()) {
    std::exit(0);
  }

#ifdef CAF_ENABLE_AFL_PERSIST
  }
#endif
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
