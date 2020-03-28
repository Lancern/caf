#ifndef CAF_V8_TRAITS_H
#define CAF_V8_TRAITS_H

#include "v8.h"

namespace caf {

class V8ArrayBuilder;

/**
 * @brief Trait type describing V8's type system.
 *
 */
struct V8Traits {
  using ValueType         = v8::Local<v8::Value>;
  using UndefinedType     = v8::Local<v8::Primitive>;
  using NullType          = v8::Local<v8::Primitive>;
  using BooleanType       = v8::Local<v8::Boolean>;
  using StringType        = v8::Local<v8::String>;
  using IntegerType       = v8::Local<v8::Integer>;
  using FloatType         = v8::Local<v8::Number>;
  using ArrayType         = v8::Local<v8::Array>;
  using ArrayBuilderType  = V8ArrayBuilder;
  using FunctionType      = v8::Local<v8::Function>;

  using ApiFunctionPtrType = v8::FunctionCallback;
}; // struct V8Traits

} // namespace caf

#endif
