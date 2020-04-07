#include "Targets/V8/V8PropertyResolver.h"

namespace caf {

Optional<typename V8Traits::ValueType>
V8PropertyResolver::Resolve(ValueType value, const std::string &name) {
  if (!value->IsObject()) {
    return Optional<typename V8Traits::ValueType> { };
  }

  v8::EscapableHandleScope handleScope { _context->GetIsolate() };

  auto objValue = value.As<v8::Object>();
  auto key = v8::String::NewFromUtf8(_context->GetIsolate(), name.c_str());
  auto prop = objValue->Get(_context, key);
  if (prop.IsEmpty()) {
    return Optional<typename V8Traits::ValueType> { };
  } else {
    return Optional<typename V8Traits::ValueType> { handleScope.Escape(prop.ToLocalChecked()) };
  }
}

} // namespace caf
