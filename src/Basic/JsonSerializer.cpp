#include "Infrastructure/Intrinsic.h"
#include "Infrastructure/Casting.h"
#include "Basic/CAFStore.h"
#include "Basic/Type.h"
#include "Basic/NamedType.h"
#include "Basic/BitsType.h"
#include "Basic/PointerType.h"
#include "Basic/ArrayType.h"
#include "Basic/StructType.h"
#include "Basic/FunctionType.h"
#include "Basic/Function.h"
#include "Basic/Constructor.h"
#include "Basic/JsonSerializer.h"

namespace caf {

void JsonSerializer::Serialize(const CAFStore& object, nlohmann::json& json) const {
  auto typesJsonArr = nlohmann::json::array();
  for (const auto& t : object.types()) {
    auto typeJson = nlohmann::json::object();
    Serialize(*t, typeJson);
    typesJsonArr.push_back(std::move(typeJson));
  }
  json["types"] = std::move(typesJsonArr);

  auto funcsJsonArr = nlohmann::json::array();
  for (const auto& f : object.funcs()) {
    auto funcJson = nlohmann::json::object();
    Serialize(*f, funcJson);
    funcsJsonArr.push_back(std::move(funcJson));
  }
  json["apis"] = std::move(funcsJsonArr);
}

void JsonSerializer::Serialize(const Type& object, nlohmann::json& json) const {
  json["kind"] = static_cast<int>(object.kind());
  json["id"] = object.id();

  if (caf::is_a<NamedType>(object)) {
    Serialize(caf::dyn_cast<NamedType>(object), json);
  } else if (caf::is_a<PointerType>(object)) {
    Serialize(caf::dyn_cast<PointerType>(object), json);
  } else if (caf::is_a<ArrayType>(object)) {
    Serialize(caf::dyn_cast<ArrayType>(object), json);
  } else if (caf::is_a<FunctionType>(object)) {
    Serialize(caf::dyn_cast<FunctionType>(object), json);
  } else {
    CAF_UNREACHABLE;
  }
}

void JsonSerializer::Serialize(const NamedType& object, nlohmann::json& json) const {
  json["name"] = object.name();

  if (caf::is_a<BitsType>(object)) {
    Serialize(caf::dyn_cast<BitsType>(object), json);
  } else if (caf::is_a<StructType>(object)) {
    Serialize(caf::dyn_cast<StructType>(object), json);
  } else {
    CAF_UNREACHABLE;
  }
}

void JsonSerializer::Serialize(const BitsType& object, nlohmann::json& json) const {
  json["size"] = object.size();
}

void JsonSerializer::Serialize(const PointerType& object, nlohmann::json& json) const {
  auto pointee = nlohmann::json();
  Serialize(object.pointeeType(), pointee);
  json["pointee"] = std::move(pointee);
}

void JsonSerializer::Serialize(const ArrayType& object, nlohmann::json& json) const {
  json["size"] = object.size();

  auto element = nlohmann::json();
  Serialize(object.elementType(), element);
  json["element"] = element;
}

void JsonSerializer::Serialize(const StructType& object, nlohmann::json& json) const {
  auto ctorsJsonArr = nlohmann::json::array();
  for (const auto& ctor : object.ctors()) {
    auto ctorJson = nlohmann::json::object();
    Serialize(ctor, ctorJson);
    ctorsJsonArr.push_back(std::move(ctorJson));
  }
  json["ctors"] = std::move(ctorsJsonArr);
}

void JsonSerializer::Serialize(const FunctionType& object, nlohmann::json& json) const {
  auto signatureJson = nlohmann::json::object();
  Serialize(object.signature(), signatureJson);
  json["signature"] = std::move(signatureJson);
  json["signatureId"] = object.signatureId();
}

void JsonSerializer::Serialize(const FunctionSignature& object, nlohmann::json& json) const {
  auto argsJsonArr = nlohmann::json::array();
  for (auto arg : object.args()) {
    auto argJson = nlohmann::json();
    Serialize(arg, argJson);
    argsJsonArr.push_back(std::move(argJson));
  }
  json["args"] = std::move(argsJsonArr);

  auto retTypeJson = nlohmann::json();
  Serialize(object.returnType(), retTypeJson);
  json["ret"] = std::move(retTypeJson);
}

void JsonSerializer::Serialize(const Function& object, nlohmann::json& json) const {
  json["id"] = object.id();
  json["name"] = object.name();

  auto signatureJson = nlohmann::json::object();
  Serialize(object.signature(), signatureJson);
  json["signature"] = std::move(signatureJson);
}

void JsonSerializer::Serialize(const Constructor& object, nlohmann::json& json) const {
  json["id"] = object.id();

  auto signatureJson = nlohmann::json::object();
  Serialize(object.signature(), signatureJson);
  json["signature"] = std::move(signatureJson);
}

} // namespace caf
