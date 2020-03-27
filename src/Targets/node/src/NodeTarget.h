#ifndef CAF_NODE_TARGET_H
#define CAF_NODE_TARGET_H

#include <v8.h>

#include <cstdint>

namespace caf {

void caf_init(char * argv[]);

// template<typename T>
class CAFFunctionCallbackInfo : public v8::FunctionCallbackInfo<v8::Value> {
public:
  CAFFunctionCallbackInfo(int8_t** implicit_args, int8_t** values, int length);
  CAFFunctionCallbackInfo(const v8::FunctionCallbackInfo<v8::Value>& info);

  v8::internal::Address* implicitArgs() { return implicit_args_; }

  int8_t** cafImplicitArgs() {return reinterpret_cast<int8_t**>(implicit_args_); }
};

v8::Local<v8::Integer> caf_CreateInteger(int32_t value);

void caf_ShowInteger(v8::Local<v8::Integer> value);

v8::Local<v8::Number> caf_CreateNumber(double value);

void caf_ShowNumber(v8::Local<v8::Number> value);

v8::Local<v8::String> caf_CreateString(char* value);

void caf_ShowString(v8::Local<v8::String> value);

v8::Local<v8::Boolean> caf_CreateBoolean(bool value);

void caf_ShowBoolean(v8::Local<v8::Boolean> value);

v8::Local<v8::Primitive> caf_CreateUndefined();

v8::Local<v8::Primitive> caf_CreateNull();

v8::Local<v8::Array> caf_CreateArray(int8_t** elements, int size);

v8::Local<v8::Function> caf_CreateFunction(int8_t* func) ;

v8::FunctionCallbackInfo<v8::Value> caf_CreateFunctionCallbackInfo(
    int8_t** implicit_args, int8_t** values, int length);

int8_t* caf_GetRetValue(v8::FunctionCallbackInfo<v8::Value>& functionCallbackInfo);

} // namespace caf

#endif
