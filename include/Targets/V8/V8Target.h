#ifndef CAF_V8_TARGET_H
#define CAF_V8_TARGET_H

#include "Targets/Common/Target.h"
#include "Targets/V8/V8Traits.h"

#include "v8.h"

namespace caf {

/**
 * @brief Provide some target-specific utility functions.
 *
 */
struct V8Target {

  /**
  * @brief Populate the function database, using the parameters wrapped in the giben function
  * callback info instance.
  *
  * @param target the target.
  * @param args the arguments.
  */
  static void PopulateFunctionDatabase(
      Target<V8Traits>& target,
      const v8::FunctionCallbackInfo<v8::Value>& args);

}; // struct V8Target

} // namespace caf

#endif
