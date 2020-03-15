#ifndef CAF_CAF_STORE_H
#define CAF_CAF_STORE_H

#include "Infrastructure/Random.h"
#include "Basic/Function.h"

#include "json/json.hpp"

#include <cassert>
#include <vector>

namespace caf {

/**
 * @brief CAF metadata store.
 *
 */
class CAFStore {
public:
  /**
   * @brief Provide statistics about the store.
   *
   */
  struct Statistics {
    size_t ApiFunctionsCount;
  }; // struct Statistics

  /**
   * @brief Construct a new CAFStore object.
   *
   */
  explicit CAFStore() = default;

  /**
   * @brief Construct a new CAFStore object.
   *
   * This constructor will trigger an assertion failure if some of the function IDs do not equal to
   * the corresponding indexes.
   *
   * @param functions the API functions.
   */
  explicit CAFStore(std::vector<Function> functions);

  /**
   * @brief Deserialize a CAFStore object from the given JSON container.
   *
   * @param json the JSON container.
   */
  explicit CAFStore(const nlohmann::json& json);

  CAFStore(const CAFStore &) = delete;
  CAFStore(CAFStore &&) noexcept = default;

  CAFStore& operator=(const CAFStore &) = delete;
  CAFStore& operator=(CAFStore &&) = default;

  /**
   * @brief Get all API functions.
   *
   * @return const std::vector<Function>& all API functions.
   */
  const std::vector<Function>& funcs() const { return _funcs; }

  /**
   * @brief Get the number of API functions.
   *
   * @return size_t the number of API functions.
   */
  size_t GetFunctionsCount() const { return _funcs.size(); }

  /**
   * @brief Add the given API function to the store.
   *
   * The ID of the given function must equal to the number of API functions registered in this
   * CAFStore at the time of being invoked, or this function will trigger an assertion failure.
   *
   * @param func the function to register.
   */
  void AddFunction(Function func) {
    assert(func.id() == _funcs.size() && "ID of the function is not valid.");
    _funcs.push_back(std::move(func));
  }

  /**
   * @brief Get the API function with the given ID.
   *
   * This function will throw an exception if the given ID cannot be found.
   *
   * @param id ID of the function.
   * @return const Function& the function.
   */
  const Function& GetFunction(FunctionIdType id) const { return _funcs.at(id); }

  /**
   * @brief Randomly select a function from this metadata store, using the given random number
   * generator.
   *
   * @param rnd the random number generator.
   * @return const Function& the function selected.
   */
  const Function& SelectFunction(Random<>& rnd) const { return rnd.Select(_funcs); }

  /**
   * @brief Get the statistics about the current store.
   *
   * @return Statistics the statistics about the current store.
   */
  Statistics GetStatistics() const;

  /**
   * @brief Serialize this object to JSON form.
   *
   * @return nlohmann::json the JSON form.
   */
  nlohmann::json ToJson() const;

private:
  std::vector<Function> _funcs;
}; // class CAFStore

} // namespace caf

#endif
