#ifndef CAF_CONSTRUCTOR_H
#define CAF_CONSTRUCTOR_H

#include "Basic/Identity.h"
#include "Basic/Function.h"
#include "Basic/CAFStore.h"

namespace caf {

/**
 * @brief A constructor of some struct type.
 */
class Constructor {
public:
  /**
   * @brief Construct a new Constructor object.
   *
   * @param signature the signature of the constructor.
   */
  explicit Constructor(FunctionSignature signature) noexcept
    : _id { },
      _signature(std::move(signature))
  { }

  /**
   * @brief Get the ID of this constructor.
   *
   * @return uint64_t ID of this constructor.
   */
  uint64_t id() const { return _id.id(); }

  /**
   * @brief Set the ID of this constructor.
   *
   * @param id ID of this constructor.
   */
  void SetId(uint64_t id) { _id.SetId(id); }

  /**
   * @brief Get the signature of this constructor.
   *
   * @return const FunctionSignature& signature of this constructor.
   */
  const FunctionSignature& signature() const { return _signature; }

private:
  /**
   * @brief Construct a new Constructor object.
   *
   */
  explicit Constructor() noexcept
    : _id { },
      _signature { }
  { }

  Identity<Constructor, uint64_t> _id;
  FunctionSignature _signature;
};

} // namespace caf

#endif
