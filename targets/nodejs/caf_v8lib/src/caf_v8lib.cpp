#include <v8.h>
#include "libplatform/libplatform.h"
#include <node.h>
#include <node_object_wrap.h>
#include <nan.h>
#include <bits/stdc++.h>
#include "caf_v8lib.h"

using namespace node;
using namespace v8;

namespace caf_v8lib {

typedef void (*FunctionCallback)(const FunctionCallbackInfo<Value>& info);

void caf_init(char * argv[])
{
  printf("caf_init\n");
  v8::V8::InitializeICUDefaultLocation(argv[0]);
  v8::V8::InitializeExternalStartupData(argv[0]);
  platform = v8::platform::NewDefaultPlatform();
  v8::V8::InitializePlatform(platform.get());
  v8::V8::Initialize();

  v8::Isolate::CreateParams create_params;
  // Create a new Isolate and make it the current one.
  create_params.array_buffer_allocator =
      v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  isolate = v8::Isolate::New(create_params);

  ::new (reinterpret_cast<void*>(ScopeBuffer)) v8::HandleScope(isolate);
  // v8::HandleScope handle_scope(isolate);
  context = v8::Context::New(isolate);
  // v8::Context::Scope context_scope(context);
  ::new (reinterpret_cast<void*>(ContextBuffer)) v8::Context::Scope(context);
  
  auto integerValue = caf_CreateInteger(12);
  caf_ShowInteger(integerValue);

  auto numberValue = caf_CreateNumber(3.1415926535);
  caf_ShowNumber(numberValue);

  auto booleanValue = caf_CreateBoolean(true);
  caf_ShowBoolean(booleanValue);

  // auto stringValue = caf_CreateString("zys");
  // caf_ShowString(stringValue);
  // int size = 4;
  // Local<v8::Array> ret = v8::Array::New(isolate, size);
}

CafFunctionCallbackInfo::CafFunctionCallbackInfo(
    int8_t** implicit_args, 
    int8_t** values,
    int length
  ): v8::FunctionCallbackInfo<v8::Value>(
    reinterpret_cast<v8::internal::Address*>(implicit_args), 
    reinterpret_cast<v8::internal::Address*>(values), 
    length) {}


// This function returns a new array
Local<Array> NewArray(int size) {

  // We will be creating temporary handles so we use a handle scope.
  v8::EscapableHandleScope handle_scope(isolate);
  // v8::Context::Scope context_scope(context);

  // Create a new empty array.
  v8::Local<v8::Array> array = v8::Array::New(isolate, size);

  // Return the value through Escape.
  return handle_scope.Escape(array);
}

v8::Local<v8::Integer> caf_CreateInteger(int32_t value) {
  printf("caf_CreateInteger\n");
  v8::EscapableHandleScope handle_scope(isolate);
  v8::Local<v8::Integer> ret = v8::Integer::New(isolate, value);
  return handle_scope.Escape(ret); 
}

void caf_ShowInteger(v8::Local<v8::Integer> value) {
  auto intValue = value->Int32Value(context).FromJust();
  printf("%d\n", intValue);
}

v8::Local<v8::Number> caf_CreateNumber(double value) {
  printf("caf_CreateNumber\n");
  v8::EscapableHandleScope handle_scope(isolate);
  v8::Local<v8::Number> ret = v8::Number::New(isolate, value);
  return handle_scope.Escape(ret); 
}

void caf_ShowNumber(v8::Local<v8::Number> value) {
  auto numberValue = value->NumberValue(context).FromJust();
  printf("%lf\n", numberValue);
}

v8::Local<v8::String> caf_CreateString(char* value) { 
  printf("caf_CreateString\n");
  v8::EscapableHandleScope handle_scope(isolate);
  auto ret = v8::String::NewFromUtf8(isolate, value, 
    v8::NewStringType::kNormal).ToLocalChecked();
  return handle_scope.Escape(ret); 
}

// Extracts a C string from a V8 Utf8Value.
const char* ToCString(const v8::String::Utf8Value& value) {
  return *value ? *value : "<string conversion failed>";
}

void caf_ShowString(v8::Local<v8::String> value) {
  v8::String::Utf8Value utf8Value(isolate, value);
  auto strValue = ToCString(utf8Value);
  printf("%s\n", strValue);
}

v8::Local<v8::Boolean> caf_CreateBoolean(bool value) {
  printf("caf_CreateBoolean\n");
  v8::EscapableHandleScope handle_scope(isolate);
  v8::Local<v8::Boolean> ret = Boolean::New(isolate, value);
  return handle_scope.Escape(ret); 
}

void caf_ShowBoolean(v8::Local<v8::Boolean> value) {
  auto boolValue = value->BooleanValue(isolate);
  printf("%x\n", boolValue);
}

v8::Local<v8::Primitive> caf_CreateUndefined() {
  printf("caf_CreateUndefined\n");
  v8::EscapableHandleScope handle_scope(isolate);
  v8::Local<v8::Primitive> ret = v8::Undefined(isolate);
  return handle_scope.Escape(ret); 
}

v8::Local<v8::Primitive> caf_CreateNull() {
  printf("caf_CreateNull\n");
  v8::EscapableHandleScope handle_scope(isolate);
  v8::Local<v8::Primitive> ret = v8::Null(isolate);
  return handle_scope.Escape(ret); 
}

Local<Array> caf_CreateArray(int8_t** elements, int size) {

  // // We will be creating temporary handles so we use a handle scope.
  // v8::EscapableHandleScope handle_scope(isolate);
  // v8::Context::Scope context_scope(context);


  // // Create a new empty array.
  // v8::Local<v8::Array> array = v8::Array::New(isolate, 3);

  printf("caf_CreateArray\n");
  // Create a new empty array.
  auto ret = NewArray(size);
  // v8::Local<v8::Array> ret = v8::Array::New(isolate, size);
  for(int i = 0; i < size; i++) {
    Nan::Set(ret, i,
      *reinterpret_cast<v8::Local<v8::Value>*>(elements + i) 
    );
  }
  // return handle_scope.Escape(ret); 
  return ret;
}

v8::Local<v8::Function> caf_CreateFunction(int8_t* func) {
  printf("caf_CreateFunction\n");
  v8::EscapableHandleScope handle_scope(isolate);
  auto functionCallback = reinterpret_cast<FunctionCallback>(func);
  auto ret = v8::Function::New(context, functionCallback).ToLocalChecked();
  return handle_scope.Escape(ret); 
}

v8::FunctionCallbackInfo<v8::Value> caf_CreateFunctionCallbackInfo(
  int8_t** implicit_args, int8_t** values, int length
  ) { 
  printf("caf_CreateFunctionCallbackInfo\n");
  // auto isolate = v8::Isolate::GetCurrent();

  implicit_args[1] = reinterpret_cast<int8_t*>(isolate);
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

} // end namespace caf_v8lib