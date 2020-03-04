#ifndef CAF_CONSTRUCTOR_H
#define CAF_CONSTRUCTOR_H

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
   * @param id the ID of the constructor.
   */
  explicit Constructor(std::string name, FunctionSignature signature, uint64_t id) noexcept
    : _id(id),
      _name(std::move(name)),
      _signature(std::move(signature))
  { }

  /**
   * @brief Get the ID of this constructor.
   *
   * @return uint64_t ID of this constructor.
   */
  uint64_t id() const { return _id; }

  /**
   * @brief Get the name of the constructor function.
   *
   * @return const std::string& name of the constructor function.
   */
  const std::string& name() const { return _name; }

  /**
   * @brief Set the ID of this constructor.
   *
   * @param id ID of this constructor.
   */
  void SetId(uint64_t id) { _id = id; }

  /**
   * @brief Get the signature of this constructor.
   *
   * @return const FunctionSignature& signature of this constructor.
   */
  const FunctionSignature& signature() const { return _signature; }

  /**
   * @brief Get the number of arguments.
   *
   * @return size_t the number of arguments.
   */
  size_t GetArgCount() const { return _signature.GetArgCount(); }

  /**
   * @brief Get the type of the argument at the given index.
   *
   * @param index the index of the argument.
   * @return CAFStoreRef<Type> the type of the argument at the given index.
   */
  CAFStoreRef<Type> GetArgType(size_t index) const { return _signature.GetArgType(index); }

private:
  uint64_t _id;
  std::string _name;
  FunctionSignature _signature;
};

} // namespace caf

#endif
