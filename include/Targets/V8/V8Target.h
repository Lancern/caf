#ifndef CAF_V8_TARGET_H
#define CAF_V8_TARGET_H

#include "Targets/V8/V8Traits.h"

namespace caf {

/**
 * @brief Get the pointer to the API function with the given ID.
 *
 * @param funcId the function ID.
 * @return V8Traits::ApiFunctionPtrType pointer to the function.
 */
typename V8Traits::ApiFunctionPtrType GetApiFunction(uint32_t funcId);

} // namespace caf

#endif
