#ifndef CAF_FUNCTION_TYPE_H
#define CAF_FUNCTION_TYPE_H

#include "Basic/Type.h"
#include "Basic/Function.h"

#include <cstdint>
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
   * @param signatureId the ID of the function signature.
   * @param id the ID of this type.
   */
  explicit FunctionType(CAFStore* store, uint64_t signatureId, uint64_t id)
    : Type { store, TypeKind::Function, id },
      _signature { },
      _signatureId(signatureId)
  { }

  /**
   * @brief Construct a new FunctionType object.
   *
   * @param store the @see CAFStore object holding this type definition.
   * @param signature the signature of the function type.
   * @param signatureId the ID of the signature.
   * @param id the ID of this type.
   */
  explicit FunctionType(
      CAFStore* store, FunctionSignature signature, uint64_t signatureId, uint64_t id)
    : Type { store, TypeKind::Function, id },
      _signature { std::move(signature) },
      _signatureId(signatureId)
  { }

  /**
   * @brief Get the function signature.
   *
   * @return const FunctionSignature& the function signature.
   */
  const FunctionSignature& signature() const { return _signature; }

  /**
   * @brief Set the function signature.
   *
   * @param signature the function signature.
   */
  void SetSigature(FunctionSignature signature) {
    _signature = std::move(signature);
  }

  /**
   * @brief Get the ID of the function signature.
   *
   * @return uint64_t ID of the function signature.
   */
  uint64_t signatureId() const { return _signatureId; }

#ifdef CAF_LLVM
  static bool classof(const Type* object) {
    return object->kind() == TypeKind::Function;
  }
#endif

private:
  FunctionSignature _signature;
  uint64_t _signatureId;
}; // class FunctionType

} // namespace caf

#endif
