#include "Infrastructure/Intrinsic.h"
#include "Basic/CAFStore.h"
#include "Fuzzer/JavaScriptSynthesisBuilder.h"
#include "Fuzzer/Value.h"

#include <cassert>
#include <cctype>

namespace caf {

namespace {

char ToHexDigit(int value) {
  assert(value >= 0 && value < 16 && "value is out of range.");
  if (value < 10) {
    return '0' + value;
  } else {
    return 'a' + (value - 10);
  }
}

} // namespace <anonymous>

void JavaScriptSynthesisBuilder::WriteLiteralValue(const Value* value) {
  assert(value && "value cannot be nullptr.");
  assert(value->kind() != ValueKind::Array && value->kind() != ValueKind::Placeholder &&
      "Array values and placeholder values cannot be written as literal.");

  auto& output = GetOutput();
  switch (value->kind()) {
    case ValueKind::Undefined:
      output << "undefined";
      break;
    case ValueKind::Null:
      output << "null";
      break;
    case ValueKind::Boolean:
      if (value->GetBooleanValue()) {
        output << "true";
      } else {
        output << "false";
      }
      break;
    case ValueKind::Integer:
      output << value->GetIntegerValue();
      break;
    case ValueKind::Float:
      output << value->GetFloatValue();
      break;
    case ValueKind::String:
      output << EscapeString(value->GetStringValue());
      break;
    case ValueKind::Function:
      output << _store.GetFunction(value->GetFunctionId()).name();
      break;
    default:
      CAF_UNREACHABLE;
  }
}

void JavaScriptSynthesisBuilder::WriteVariableDef(
    const std::string& varName,
    const Value* value) {
  auto& output = GetOutput();
  output << "let " << varName << " = ";
  WriteLiteralValue(value);
}

void JavaScriptSynthesisBuilder::WriteEmptyArrayVariableDef(const std::string& varName) {
  auto& output = GetOutput();
  output << "let " << varName << " = [];";
}

void JavaScriptSynthesisBuilder::WriteArrayPushStatement(
    const std::string& varName,
    const std::string &elementVarName) {
  auto& output = GetOutput();
  output << varName << ".push(" << elementVarName << ");";
}

void JavaScriptSynthesisBuilder::WriteFunctionCallStatement(
    const std::string& retVarName,
    const std::string& functionName,
    const std::string& receiverVarName,
    const std::vector<std::string>& argVarNames) {
  auto& output = GetOutput();
  output << "let " << retVarName << " = " << functionName;

  if (!receiverVarName.empty()) {
    // Special receiver is specified.
    output << ".apply";
  }

  output << "(";

  auto firstArg = true;
  if (!receiverVarName.empty()) {
    output << receiverVarName;
    firstArg = false;
  }

  for (const auto& argVarName : argVarNames) {
    if (firstArg) {
      firstArg = false;
    } else {
      output << ", ";
    }
    output << argVarName;
  }

  output << ");";
}

std::string JavaScriptSynthesisBuilder::EscapeString(const std::string& s) const {
  std::string ret;
  ret.reserve(s.size());

  for (auto ch : s) {
    if (std::isprint(ch)) {
      if (ch == '\"') {
        ret.append("\\\"");
      } else if (ch == '\'') {
        ret.append("\\\'");
      } else {
        ret.push_back(ch);
      }
    } else {
      if (ch == '\n') {
        ret.append("\\n");
      } else if (ch == '\t') {
        ret.append("\\t");
      } else if (ch == '\r') {
        ret.append("\\r");
      } else {
        ret.append("\\x");
        ret.push_back(ToHexDigit(ch / 16));
        ret.push_back(ToHexDigit(ch % 16));
      }
    }
  }

  return ret;
}

} // namespace caf
