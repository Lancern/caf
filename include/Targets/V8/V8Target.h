#ifndef CAF_V8_TARGET_H
#define CAF_V8_TARGET_H

#include "Infrastructure/Optional.h"

#include "v8.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <vector>

namespace caf {

/**
 * @brief Provide convenient ways to create instances of v8::FunctionCallbackInfo.
 *
 */
class FunctionCallbackInfoBuilder {
public:
  /**
   * @brief Construct a new FunctionCallbackInfoBuilder object.
   *
   */
  explicit FunctionCallbackInfoBuilder() {
    std::fill(std::begin(_implicitArgs), std::end(_implicitArgs), v8::internal::kNullAddress);
  }

  /**
   * @brief Clone this object.
   *
   * @return FunctionCallbackInfoBuilder the cloned object.
   */
  FunctionCallbackInfoBuilder Clone() const { return *this; }

  /**
   * @brief Build a new v8::FunctionCallbackInfo<v8::Value> object and returns a reference to it.
   *
   * The created object is managed by this object.
   *
   * @return v8::FunctionCallbackInfo<v8::Value>& reference to the created object.
   */
  v8::FunctionCallbackInfo<v8::Value>& Build() {
    _info.emplace(_implicitArgs, _args.data(), _args.size());
    return _info.value();
  }

  /**
   * @brief Set the implicit arguments.
   *
   * @param implicitArgs the implicit arguments.
   */
  void SetImplicitArgs(v8::internal::Address* implicitArgs) {
    std::copy(implicitArgs, implicitArgs + ImplicitArgsCount, _implicitArgs);
  }

  /**
   * @brief Clear all explicit arguments.
   *
   */
  void ClearArgs() { _args.clear(); }

  /**
   * @brief Add an explicit argument to the function call.
   *
   * @param arg the argument to add.
   */
  void AddArgument(v8::Local<v8::Value> arg);

private:
  class MockFunctionCallbackInfo : public v8::FunctionCallbackInfo<v8::Value> {
  public:
    explicit MockFunctionCallbackInfo(
        v8::internal::Address* implicitArgs, v8::internal::Address* argv, int argc)
      : v8::FunctionCallbackInfo<v8::Value> { implicitArgs, argv, argc }
    { }
  };

  constexpr static const int ImplicitArgsCount = 6;

  v8::internal::Address _implicitArgs[ImplicitArgsCount];
  std::vector<v8::internal::Address> _args;

  Optional<MockFunctionCallbackInfo> _info;
}; // class FunctionCallbackInfoBuilder

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
