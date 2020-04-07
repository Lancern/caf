#include "Basic/CAFStore.h"

#include <utility>

namespace caf {

CAFStore::CAFStore(std::vector<Function> functions)
  : _funcs(std::move(functions))
{ }

CAFStore::CAFStore(const nlohmann::json& json) {
  _funcs.reserve(json.size());
  FunctionIdType id = 0;
  for (const auto& funcName : json) {
    _funcs.emplace_back(id++, funcName.get<std::string>());
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
    json.push_back(nlohmann::json(func.name()));
  }
  return json;
}

} // namespace caf
