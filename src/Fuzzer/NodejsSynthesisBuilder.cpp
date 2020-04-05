#include "Basic/CAFStore.h"
#include "Fuzzer/NodejsSynthesisBuilder.h"
#include "Fuzzer/Value.h"

#include <cctype>

namespace caf {

namespace {

bool IsInModule(const std::string& name) {
  if (name.empty()) {
    return false;
  }

  if (!std::islower(name[0])) {
    return false;
  }

  if (name.find('.') == std::string::npos) {
    return false;
  }

  return true;
}

std::string GetModuleName(const std::string& name) {
  return name.substr(0, name.find('.'));
}

} // namespace <anonymous>

void NodejsSynthesisBuilder::WriteRequireStatement(const std::string& moduleName) {
  if (_imported.find(moduleName) != _imported.end()) {
    // The same module has already been imported.
    return;
  }

  auto& output = GetOutput();
  output << "let " << moduleName << " = require(\'" << moduleName << "\');";

  _imported.insert(moduleName);
}

void NodejsSynthesisBuilder::WriteVariableDef(const std::string &varName, const Value *value) {
  assert(value && "value cannot be nullptr.");
  if (value->kind() == ValueKind::Function) {
    const auto& funcName = store().GetFunction(value->GetFunctionId()).name();
    if (IsInModule(funcName)) {
      WriteRequireStatement(GetModuleName(funcName));
    }
  }

  JavaScriptSynthesisBuilder::WriteVariableDef(varName, value);
}

void NodejsSynthesisBuilder::WriteFunctionCallStatement(
    const std::string& retVarName,
    const std::string& functionName,
    const std::string& receiverVarName,
    const std::vector<std::string>& argVarNames) {
  if (IsInModule(functionName)) {
    WriteRequireStatement(GetModuleName(functionName));
  }

  JavaScriptSynthesisBuilder::WriteFunctionCallStatement(
      retVarName, functionName, receiverVarName, argVarNames);
}

} // namespace caf
