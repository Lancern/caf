#ifndef CAF_V8_LIB_H
#define CAF_V8_LIB_H
#include <v8.h>
#include "libplatform/libplatform.h"
#include <node.h>
#include <node_object_wrap.h>
#include <nan.h>
#include "caf_v8lib.h"
#include <new>

using namespace node;
using namespace v8;

namespace caf_v8lib {

using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::NewStringType;
using v8::Object;
using v8::String;
using v8::Value;
using v8::Integer;
using v8::Number;
using v8::Boolean;
using v8::Undefined;
using v8::Array;
using v8::Context;

typedef void (*FunctionCallback)(const FunctionCallbackInfo<Value>& info);

v8::Isolate* isolate;
v8::Local<v8::Context> context;

alignas(alignof(v8::HandleScope))
char ScopeBuffer[sizeof(v8::HandleScope)];

alignas(alignof(v8::Context))
char ContextBuffer[sizeof(v8::Context)];

void caf_init(char * argv[]);

// template<typename T>
class CafFunctionCallbackInfo : public v8::FunctionCallbackInfo<v8::Value> {
public:
  CafFunctionCallbackInfo(
    int8_t** implicit_args, 
    int8_t** values,
    int length
  );
};

Local<Integer> caf_CreateInteger(int32_t value);

void caf_ShowInteger(Local<Integer> value);

Local<Number> caf_CreateNumber(double value);

void caf_ShowNumber(Local<Number> value);

Local<String> caf_CreateString(char* value);

void caf_ShowString(Local<String> value);

Local<Boolean> caf_CreateBoolean(bool value);

void caf_ShowBoolean(Local<Boolean> value);

Local<Primitive> caf_CreateUndefined();

Local<Primitive> caf_CreateNull();

Local<Array> caf_CreateArray(int8_t** elements, int size);

Local<Function> caf_CreateFunction(int8_t* func) ;

v8::FunctionCallbackInfo<v8::Value> caf_CreateFunctionCallbackInfo(
  int8_t** implicit_args, int8_t** values, int length
  );

int8_t* caf_GetRetValue(v8::FunctionCallbackInfo<v8::Value>& functionCallbackInfo);
} // end namespace caf_v8lib

#endif