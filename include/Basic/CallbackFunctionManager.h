#ifndef CAF_CALLBACK_FUNCTION_MAP_H
#define CAF_CALLBACK_FUNCTION_MAP_H

#include "Basic/Identity.h"

#include <memory>
#include <vector>
#include <unordered_map>

namespace caf {

class FunctionSignature;

/**
 * @brief Identifier of a callback function, consisting of the signature ID and the function ID.
 *
 */
struct CallbackFunctionIdentifier {
  /**
   * @brief The callback function's signature ID.
   *
   */
  int SignatureId;

  /**
   * @brief The callback function's function ID.
   *
   */
  int FunctionId;
}; // struct CallbackFunctionIdentifier

/**
 * @brief A data structure that support efficient grouping of callback functions by their
 * signatures.
 *
 */
class CallbackFunctionManager {
public:
  /**
   * @brief Insert a new callback function whose signature is the given signature into this
   * @see CallbackFunctionManager object.
   *
   * @param signature the signature of the callback function.
   * @return CallbackFunctionIdentifier the identifier of the callback function. The function
   * identifiers are guaranteed to be distinct and within range [0, n) where n is the number of
   * functions inserted into the @see CallbackFunctionManager object.
   */
  CallbackFunctionIdentifier Insert(const FunctionSignature& signature);

private:
  class FunctionSignatureSet;

  // _slots[i] is the list of callback function IDs whose signature ID is i.
  std::unordered_map<int, std::vector<int>> _slots;
  std::unique_ptr<FunctionSignatureSet> _signatures;
  IncrementIdAllocator<int> _funcIdAlloc;
}; // class CallbackFunctionManager

} // namespace caf

#endif
