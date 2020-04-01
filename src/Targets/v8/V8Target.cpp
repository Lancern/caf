#include "Targets/V8/V8Target.h"

namespace caf {

void V8Target::PopulateFunctionDatabase(
    Target<V8Traits> &target,
    const v8::FunctionCallbackInfo<v8::Value> &args) {
  auto isolate = args.GetIsolate();
  v8::EscapableHandleScope handleScope { isolate };
  auto context = isolate->GetCurrentContext();

  assert(args.Length() == 1 && "Invalid number of arguments.");
  assert(args[0]->IsArray() && "Invalid argument: not an array.");

  auto funcs = args[0].As<v8::Array>();
  auto& db = target.functions();
  for (size_t i = 0; i < funcs->Length(); ++i) {
    auto element = funcs->Get(context, i).ToLocalChecked();
    assert(element->IsObject() && "Invalid argument: element is not an object.");

    auto elementObj = element.As<v8::Object>();
    auto idObj = elementObj->Get(
        context,
        v8::String::NewFromUtf8(isolate, "id", v8::NewStringType::kNormal).ToLocalChecked())
      .ToLocalChecked();
    assert(idObj->IsUint32() && "Invalid function ID.");
    auto id = idObj->ToUint32(context).ToLocalChecked()->Value();

    auto funcObj = elementObj->Get(
        context,
        v8::String::NewFromUtf8(isolate, "func", v8::NewStringType::kNormal).ToLocalChecked())
      .ToLocalChecked();
    assert(funcObj->IsFunction() && "Invalid function object.");
    auto func = funcObj.As<v8::Function>();

    db.AddFunction(id, handleScope.Escape(func));
  }
}

} // namespace caf
