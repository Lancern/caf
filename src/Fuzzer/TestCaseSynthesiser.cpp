#include "Basic/CAFStore.h"
#include "Fuzzer/SynthesisBuilder.h"
#include "Fuzzer/TestCase.h"
#include "Fuzzer/TestCaseSynthesiser.h"

namespace caf {

void TestCaseSynthesiser::Synthesis(const TestCase &tc) {
  _builder.EnterMainFunction();

  _retValVars.reserve(tc.GetFunctionCallsCount());
  for (const auto& call : tc) {
    auto receiver = SynthesisVariable::Empty();
    if (call.HasThis()) {
      receiver = SynthesisValue(call.GetThis());
    }

    std::vector<SynthesisVariable> args;
    args.reserve(call.GetArgsCount());

    for (auto arg : call) {
      args.push_back(SynthesisValue(arg));
    }

    const auto& functionName = _store.GetFunction(call.funcId()).name();
    _retValVars.push_back(_builder.SynthesisFunctionCall(functionName, receiver, args));
  }

  _builder.LeaveFunction();
}

std::string TestCaseSynthesiser::GetCode() const {
  return _builder.GetCode();
}

SynthesisVariable TestCaseSynthesiser::SynthesisValue(const Value* value) {
  if (value->IsPlaceholder()) {
    return _retValVars.at(value->GetPlaceholderIndex());
  } else {
    return _builder.SynthesisConstant(value);
  }
}

} // namespace caf
