#ifndef CAF_FUNCTION_TYPE_H
#define CAF_FUNCTION_TYPE_H

#include "Basic/Type.h"
#include "Basic/Function.h"

#include <utility>

namespace caf {

class CAFStore;

/**
 * @brief Represent a function type.
 *
 */
class FunctionType : public Type {
public:
  /**
   * @brief Construct a new FunctionType object.
   *
   * @param store the @see CAFStore object holding this type definition.
   * @param signature the function signature of this function type.
   */
  explicit FunctionType(CAFStore* store, FunctionSignature signature)
    : Type { store, TypeKind::Function },
      _signature(std::move(signature))
  { }

  /**
   * @brief Get the function signature.
   *
   * @return const FunctionSignature& the function signature.
   */
  const FunctionSignature& signature() const { return _signature; }

#ifdef CAF_LLVM
  static bool classof(const Type* object) {
    return object->kind() == TypeKind::Function;
  }
#endif

private:
  FunctionSignature _signature;
}; // class FunctionType

} // namespace caf

#endif
