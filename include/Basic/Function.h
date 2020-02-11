#ifndef CAF_FUNCTION_H
#define CAF_FUNCTION_H

#include <vector>

#include "Infrastructure/Hash.h"
#include "Basic/Identity.h"
#include "Basic/CAFStore.h"

#include "json/json.hpp"

namespace caf {

class CAFStore;
class Type;

/**
 * @brief Represents signature of a function-like object.
 *
 */
class FunctionSignature {
public:
  /**
   * @brief Construct a new FunctionSignature object.
   *
   * @param returnType the return type of the function.
   * @param args the types of arguments of the function.
   */
  explicit FunctionSignature(CAFStoreRef<Type> returnType, std::vector<CAFStoreRef<Type>> args)
    : _returnType(returnType),
      _args(std::move(args))
  { }

  /**
   * @brief Construct a new FunctionSignature object.
   *
   */
  explicit FunctionSignature()
    : _returnType { },
      _args { }
  { }

  FunctionSignature(const FunctionSignature &) = delete;
  FunctionSignature(FunctionSignature &&) noexcept = default;

  FunctionSignature& operator=(const FunctionSignature &) = delete;
  FunctionSignature& operator=(FunctionSignature &&) noexcept = default;

  /**
   * @brief Get the return type of the function.
   *
   * @return CAFStoreRef<Type> the return type of the function.
   */
  CAFStoreRef<Type> returnType() const { return _returnType; }

  /**
   * @brief Get the types of arguments of the function.
   *
   * @return const std::vector<CAFStoreRef<Type>> & type of arguments of the function.
   */
  const std::vector<CAFStoreRef<Type>>& args() const { return _args; }

  /**
   * @brief Get the number of arguments.
   *
   * @return size_t the number of arguments.
   */
  size_t GetArgCount() const { return _args.size(); }

  /**
   * @brief Get the type of the argument at the given index.
   *
   * @param index the index of the argument.
   * @return CAFStoreRef<Type> the type of the argument.
   */
  CAFStoreRef<Type> GetArgType(size_t index) const { return _args[index]; }

private:
  CAFStoreRef<Type> _returnType;
  std::vector<CAFStoreRef<Type>> _args;
};

/**
 * @brief Represent an API function.
 *
 */
class Function {
public:
  /**
   * @brief Construct a new Function object.
   *
   * @param store the store holding the Function object.
   * @param name the name of the function.
   * @param signature the signature of the function.
   */
  explicit Function(CAFStore* store, std::string name, FunctionSignature signature)
    : _store(store),
      _id { },
      _name(std::move(name)),
      _signature(std::move(signature))
  { }

  /**
   * @brief Get the @see CAFStore owning this object.
   *
   * @return CAFStore* the @see CAFStore object owning this object.
   */
  CAFStore* store() const { return _store; }

  /**
   * @brief Get the ID of this API function.
   *
   * @return uint64_t ID of this API function.
   */
  uint64_t id() const { return _id.id(); }

  /**
   * @brief Set ID of this function.
   *
   * @param id new ID of this function.
   */
  void SetId(uint64_t id) { _id.SetId(id); }

  /**
   * @brief Get the name of the function.
   *
   * @return const std::string& name of the function.
   */
  const std::string& name() const { return _name; }

  /**
   * @brief Get the signature of this function.
   *
   * @return const FunctionSignature& signature of this function.
   */
  const FunctionSignature& signature() const { return _signature; }

private:
  /**
   * @brief Construct a new Function object.
   *
   * @param store the CAFStore object holding this object.
   */
  explicit Function(CAFStore* store)
    : _store(store),
      _id { },
      _name { },
      _signature { }
  { }

  CAFStore* _store;
  Identity<Function, uint64_t> _id;
  std::string _name;
  FunctionSignature _signature;
};

} // namespace caf

#endif
