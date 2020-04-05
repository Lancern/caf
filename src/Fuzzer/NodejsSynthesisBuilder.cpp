#include "Basic/CAFStore.h"
#include "Fuzzer/NodejsSynthesisBuilder.h"
#include "Fuzzer/Value.h"

#include <cctype>
#include <unordered_set>

namespace caf {

namespace {

static const std::unordered_set<std::string> NativeModuleNames = {
  "async_hooks",
  "buffer",
  "child_process",
  "cluster",
  "console",
  "constants",
  "crypto",
  "dgram",
  "dns",
  "domain",
  "events",
  "fs",
  "http",
  "http2",
  "https",
  "inspector",
  "module",
  "net",
  "os",
  "path",
  "perf_hooks",
  "process",
  "punycode",
  "querystring",
  "readline",
  "repl",
  "stream",
  "string_decoder",
  "sys",
  "timers",
  "tls",
  "trace_events",
  "tty",
  "url",
  "util",
  "v8",
  "vm",
  "worker_threads",
  "zlib"
};

bool IsInModule(const std::string& name) {
  if (name.empty()) {
    return false;
  }

  if (!std::islower(name[0])) {
    return false;
  }

  auto sepIndex = name.find('.');
  if (sepIndex == std::string::npos) {
    return false;
  }

  auto moduleName = name.substr(0, sepIndex);
  return NativeModuleNames.find(moduleName) != NativeModuleNames.end();
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
    bool isCtorCall,
    const std::string& receiverVarName,
    const std::vector<std::string>& argVarNames) {
  if (IsInModule(functionName)) {
    WriteRequireStatement(GetModuleName(functionName));
  }

  JavaScriptSynthesisBuilder::WriteFunctionCallStatement(
      retVarName, functionName, isCtorCall, receiverVarName, argVarNames);
}

} // namespace caf
