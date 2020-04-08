#include "Infrastructure/Casting.h"
#include "Fuzzer/SynthesisBuilder.h"
#include "Fuzzer/Value.h"

#include <cassert>

namespace caf {

SynthesisVariable SynthesisBuilder::SynthesisConstant(const Value *value) {
  assert(value && "value cannot be nullptr.");
  if (HasSynthesised(value)) {
    return GetSynthesisedVariable(value);
  }

  assert(value->kind() != ValueKind::Placeholder && "Cannot synthesis a placeholder value.");

  auto varName = GetNextVariableName();
  auto var = CreateVariable(varName);
  SetLastSynthesisedValue(value);

  if (value->kind() == ValueKind::Array) {
    auto arrayValue = caf::dyn_cast<ArrayValue>(value);
    WriteEmptyArrayVariableDef(varName);
    for (size_t i = 0; i < arrayValue->size(); ++i) {
      auto elementVar = SynthesisConstant(arrayValue->GetElement(i));
      WriteArrayPushStatement(varName, elementVar.GetName());
    }
  } else {
    WriteVariableDef(varName, value);
  }

  return var;
}

SynthesisVariable SynthesisBuilder::SynthesisFunctionCall(
    const std::string &functionName,
    bool isCtorCall,
    const SynthesisVariable &receiver,
    const std::vector<SynthesisVariable> &args) {
  std::string receiverVarName;
  if (!receiver.IsEmpty()) {
    receiverVarName = receiver.GetName();
  }

  std::vector<std::string> argVarNames;
  argVarNames.reserve(args.size());
  for (const auto& var : args) {
    argVarNames.push_back(var.GetName());
  }

  auto retValName = GetNextVariableName();
  WriteFunctionCallStatement(retValName, functionName, isCtorCall, receiverVarName, argVarNames);
  return CreateVariable(std::move(retValName));
}

std::string SynthesisBuilder::GetCode() const {
  return _output.str();
}

std::string SynthesisBuilder::GetNextVariableName() {
  std::string name = "_";
  name.append(std::to_string(_varId++));
  return name;
}

void SynthesisBuilder::WriteVariableRef(const std::string& varName) {
  _output << varName;
}

} // namespace caf
