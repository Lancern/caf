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

void Initialize(const v8::FunctionCallbackInfo<v8::Value>& args) {
  // SetupAbortHandler();
}

} // namespace <anonymous>

} // namespace caf

extern "C" NODE_MODULE_EXPORT void
NODE_MODULE_INITIALIZER(
    v8::Local<v8::Object> exports,
    v8::Local<v8::Value> module,
    v8::Local<v8::Context> context) {
  auto env = node::Environment::GetCurrent(context);
  env->SetMethod(exports, "init", &caf::Initialize);
}
