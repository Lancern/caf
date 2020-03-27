#include "NodeTarget.h"

#include <v8.h>
#include "libplatform/libplatform.h"
#include <node.h>
#include <node_object_wrap.h>
#include <nan.h>
#include <bits/stdc++.h>
#include <env.h>
#include <env-inl.h>

// #define CAF_ENTRY
#ifdef CAF_ENTRY

extern "C" void __caf_main();

namespace caf {

namespace node_module {

static void LaunchCAF(const v8::FunctionCallbackInfo<v8::Value>& info) {
  // TODO: Implement me.
}

// Initialize this addon to be context-aware.
NODE_MODULE_INIT(/* exports, module, context */) {
  auto isolate = context->GetIsolate();
  auto env = node::Environment::GetCurrent(isolate);
  env->SetMethod(exports, "launch", &LaunchCAF);
}

} // namespace node_module

} // namespace caf

#else // #ifdef CAF_ENTRY

namespace caf {

CAFFunctionCallbackInfo::CAFFunctionCallbackInfo(
    int8_t** implicit_args, int8_t** values, int length)
    : v8::FunctionCallbackInfo<v8::Value> {
        reinterpret_cast<v8::internal::Address*>(implicit_args),
        reinterpret_cast<v8::internal::Address*>(values),
        length }
{ }

CAFFunctionCallbackInfo::CAFFunctionCallbackInfo(const FunctionCallbackInfo<v8::Value>& info)
    : v8::FunctionCallbackInfo<v8::Value> { info }
{ }

v8::Local<v8::Integer> caf_CreateInteger(int32_t value) {
  printf("caf_CreateInteger\n");
  v8::EscapableHandleScope handle_scope(caf_isolate);
  v8::Local<v8::Integer> ret = v8::Integer::New(caf_isolate, value);
  return handle_scope.Escape(ret);
}

void caf_ShowInteger(v8::Local<v8::Integer> value) {
  auto intValue = value->Int32Value(caf_context).FromJust();
  printf("%d\n", intValue);
}

v8::Local<v8::Number> caf_CreateNumber(double value) {
  printf("caf_CreateNumber\n");
  v8::EscapableHandleScope handle_scope(caf_isolate);
  v8::Local<v8::Number> ret = v8::Number::New(caf_isolate, value);
  return handle_scope.Escape(ret);
}

void caf_ShowNumber(v8::Local<v8::Number> value) {
  auto numberValue = value->NumberValue(caf_context).FromJust();
  printf("%lf\n", numberValue);
}

v8::Local<v8::String> caf_CreateString(char* value) {
  printf("caf_CreateString\n");
  v8::EscapableHandleScope handle_scope(caf_isolate);
  auto ret = v8::String::NewFromUtf8(caf_isolate, value,
    v8::NewStringType::kNormal).ToLocalChecked();
  return handle_scope.Escape(ret);
}

// Extracts a C string from a V8 Utf8Value.
const char* ToCString(const v8::String::Utf8Value& value) {
  return *value ? *value : "<string conversion failed>";
}

void caf_ShowString(v8::Local<v8::String> value) {
  v8::String::Utf8Value utf8Value(caf_isolate, value);
  auto strValue = ToCString(utf8Value);
  printf("%s\n", strValue);
}

v8::Local<v8::Boolean> caf_CreateBoolean(bool value) {
  printf("caf_CreateBoolean\n");
  v8::EscapableHandleScope handle_scope(caf_isolate);
  v8::Local<v8::Boolean> ret = Boolean::New(caf_isolate, value);
  return handle_scope.Escape(ret);
}

void caf_ShowBoolean(v8::Local<v8::Boolean> value) {
  auto boolValue = value->BooleanValue(caf_isolate);
  printf("%x\n", boolValue);
}

v8::Local<v8::Primitive> caf_CreateUndefined() {
  printf("caf_CreateUndefined\n");
  v8::EscapableHandleScope handle_scope(caf_isolate);
  v8::Local<v8::Primitive> ret = v8::Undefined(caf_isolate);
  return handle_scope.Escape(ret);
}

v8::Local<v8::Primitive> caf_CreateNull() {
  printf("caf_CreateNull\n");
  v8::EscapableHandleScope handle_scope(caf_isolate);
  v8::Local<v8::Primitive> ret = v8::Null(caf_isolate);
  return handle_scope.Escape(ret);
}

Local<Array> caf_CreateArray(int8_t** elements, int size) {

  printf("caf_CreateArray\n");
  auto arrayElements = reinterpret_cast<Local<Value>*>(elements);
  v8::EscapableHandleScope handle_scope(caf_isolate);
  v8::Local<v8::Array> ret = v8::Array::New(caf_isolate, arrayElements, size);
  return handle_scope.Escape(ret);
}

v8::Local<v8::Function> caf_CreateFunction(int8_t* func) {
  printf("caf_CreateFunction\n");
  v8::EscapableHandleScope handle_scope(caf_isolate);
  auto functionCallback = reinterpret_cast<FunctionCallback>(func);
  auto ret = v8::Function::New(caf_context, functionCallback).ToLocalChecked();
  return handle_scope.Escape(ret);
}

v8::FunctionCallbackInfo<v8::Value> caf_CreateFunctionCallbackInfo(
  int8_t** implicit_args, int8_t** values, int length
  ) {
  printf("caf_CreateFunctionCallbackInfo\n");
  // // implicit_args[0] = <this: Value>, handled by IR imstrumentor.
  // implicit_args[1] = reinterpret_cast<int8_t*>(caf_isolate);
  // implicit_args[3] = reinterpret_cast<int8_t*>(
  //   (v8::ReturnValue<Value>*)malloc(sizeof(v8::ReturnValue<Value>)));
  // // implicit_args[4] = (int8_t*)malloc(sizeof(int8_t));
  // auto env = *reinterpret_cast<volatile internal::Address*>(*functionCallbackInfo_Data);
  // implicit_args[4] = reinterpret_cast<int8_t*>(env);
  // //   *reinterpret_cast<internal::Address*>(*caf_environment->as_callback_data());
  // implicit_args[5] = reinterpret_cast<int8_t*>(
  //   (v8::Value*)malloc(sizeof(v8::Value)));
  for(int i = 1; i < 6; i++) {
    implicit_args[i] = caf_implicit_args[i];
  }

  auto cafFunctionCallbackInfo = CafFunctionCallbackInfo(implicit_args, values, length);
  auto functionCallbackInfo = (v8::FunctionCallbackInfo<v8::Value>)(cafFunctionCallbackInfo);
  return functionCallbackInfo;
}

int8_t* caf_GetRetValue(v8::FunctionCallbackInfo<v8::Value>& functionCallbackInfo) {
  printf("caf_GetRetValue\n");
  auto returnValue = functionCallbackInfo.GetReturnValue();
  auto cafReturnValue = new v8::ReturnValue<v8::Value>(returnValue);
  return reinterpret_cast<int8_t*>(cafReturnValue);
}

} // namespace caf

#endif
