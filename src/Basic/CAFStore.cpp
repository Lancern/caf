#include "Basic/CAFStore.h"

#include <utility>

namespace caf {

CAFStore::CAFStore(std::vector<Function> functions)
  : _funcs(std::move(functions))
{ }

CAFStore::CAFStore(const nlohmann::json& json) {
  _funcs.reserve(json.size());
  for (const auto& funcJson : json) {
    _funcs.emplace_back(funcJson);
  }
}

CAFStore::Statistics CAFStore::GetStatistics() const {
  Statistics stat;
  stat.ApiFunctionsCount = _funcs.size();
  return stat;
}

nlohmann::json CAFStore::ToJson() const {
  auto json = nlohmann::json::array();
  for (const auto& func : _funcs) {
    json.push_back(func.ToJson());
  }
  return json;
}

} // namespace caf
