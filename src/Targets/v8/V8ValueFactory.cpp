#include "Targets/V8/V8ValueFactory.h"
#include "Targets/V8/V8ArrayBuilder.h"
#include "Targets/V8/V8Target.h"

#include "v8.h"


namespace caf {

#define BEGIN_MAKE_HANDLE(isolate) \
    v8::EscapableHandleScope handleScope__ { isolate }

#define END_MAKE_HANDLE(handle) \
    handleScope__.Escape(handle)

typename V8Traits::UndefinedType
V8ValueFactory::CreateUndefined() {
  BEGIN_MAKE_HANDLE(_isolate);
  auto ret = v8::Undefined(_isolate);
  return END_MAKE_HANDLE(ret);
}

typename V8Traits::NullType
V8ValueFactory::CreateNull() {
  BEGIN_MAKE_HANDLE(_isolate);
  auto ret = v8::Null(_isolate);
  return END_MAKE_HANDLE(ret);
}

typename V8Traits::BooleanType
V8ValueFactory::CreateBoolean(bool value) {
  BEGIN_MAKE_HANDLE(_isolate);
  auto ret = v8::Boolean::New(_isolate, value);
  return END_MAKE_HANDLE(ret);
}

typename V8Traits::FunctionType
V8ValueFactory::CreateFunction(uint32_t funcId) {
  auto funcPtr = reinterpret_cast<V8Traits::ApiFunctionPtrType>(GetApiFunction(funcId));
  BEGIN_MAKE_HANDLE(_isolate);
  auto ret = v8::Function::New(_context, funcPtr, _callbackData).ToLocalChecked();
  return END_MAKE_HANDLE(ret);
}

typename V8Traits::StringType
V8ValueFactory::CreateString(const uint8_t *buffer, size_t size) {
  BEGIN_MAKE_HANDLE(_isolate);
  auto ret = v8::String::NewFromUtf8(
      _isolate, reinterpret_cast<const char *>(buffer), v8::NewStringType::kNormal, size)
    .ToLocalChecked();
  return END_MAKE_HANDLE(ret);
}

typename V8Traits::IntegerType
V8ValueFactory::CreateInteger(int32_t value) {
  BEGIN_MAKE_HANDLE(_isolate);
  auto ret = v8::Integer::New(_isolate, value);
  return END_MAKE_HANDLE(ret);
}

typename V8Traits::FloatType
V8ValueFactory::CreateFloat(double value) {
  BEGIN_MAKE_HANDLE(_isolate);
  auto ret = v8::Number::New(_isolate, value);
  return END_MAKE_HANDLE(ret);
}

typename V8Traits::ArrayBuilderType
V8ValueFactory::StartBuildArray(size_t size) {
  return V8ArrayBuilder { _isolate, size };
}

} // namespace caf
