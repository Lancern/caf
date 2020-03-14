#include "Basic/Function.h"

namespace caf {

Function::Function(const nlohmann::json& json)
  : _id(json.at("id").get<FunctionIdType>()),
    _name(json.at("name").get<std::string>())
{ }

nlohmann::json Function::ToJson() const {
  auto json = nlohmann::json::object();
  json["id"] = _id;
  json["name"] = _name;
  return json;
}

} // namespace caf
