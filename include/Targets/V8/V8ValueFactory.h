#ifndef CAF_V8_VALUE_FACTORY_H
#define CAF_V8_VALUE_FACTORY_H

#include "Targets/Common/ValueFactory.h"
#include "Targets/V8/V8Traits.h"

#include "v8.h"

#include <cstddef>
#include <cstdint>

namespace caf {

/**
 * @brief Creating V8 specific values.
 *
 */
class V8ValueFactory : public ValueFactory<V8Traits> {
public:
  /**
   * @brief Construct a new V8ValueFactory object.
   *
   * @param isolate the V8 isolate instance.
   * @param context the context.
   * @param callbackData the embedder's data for callback functions.
   */
  explicit V8ValueFactory(
      v8::Isolate* isolate, v8::Local<v8::Context> context, v8::Local<v8::Value> callbackData)
    : _isolate(isolate),
      _context(context),
      _callbackData(callbackData)
  { }

  typename V8Traits::UndefinedType CreateUndefined() override;

  typename V8Traits::NullType CreateNull() override;

  typename V8Traits::BooleanType CreateBoolean(bool value) override;

  typename V8Traits::FunctionType CreateFunction(uint32_t funcId) override;

  typename V8Traits::StringType CreateString(const uint8_t* buffer, size_t size) override;

  typename V8Traits::IntegerType CreateInteger(int32_t value) override;

  typename V8Traits::FloatType CreateFloat(double value) override;

  typename V8Traits::ArrayBuilderType StartBuildArray(size_t size) override;

private:
  v8::Isolate* _isolate;
  v8::Local<v8::Context> _context;
  v8::Local<v8::Value> _callbackData;
}; // class V8ValueFactory

} // namespace caf

#endif
