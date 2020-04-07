#ifndef CAF_V8_EXECUTOR_H
#define CAF_V8_EXECUTOR_H

#include "Targets/Common/AbstractExecutor.h"
#include "Targets/V8/V8Traits.h"

#include "v8.h"

#include <vector>
#include <unordered_map>

namespace caf {

/**
 * @brief V8 specific function executor.
 *
 */
class V8Executor : public AbstractExecutor<V8Traits> {
public:
  /**
   * @brief Construct a new V8Executor object.
   *
   * @param isolate the isolate instance.
   * @param context the context.
   * @param callbackData the callback data.
   */
  explicit V8Executor(
      v8::Isolate* isolate, v8::Local<v8::Context> context, v8::Local<v8::Value> callbackData)
    : _isolate(isolate), _context(context), _callbackData(callbackData)
  { }

  typename V8Traits::ValueType Invoke(
      v8::Local<v8::Value> function,
      typename V8Traits::ValueType receiver,
      bool isCtorCall,
      std::vector<typename V8Traits::ValueType>& args) override;

private:
  v8::Isolate* _isolate;
  v8::Local<v8::Context> _context;
  v8::Local<v8::Value> _callbackData;
}; // class V8Executor

} // namespace caf

#endif
