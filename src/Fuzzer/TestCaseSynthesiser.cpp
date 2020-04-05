#include "Basic/CAFStore.h"
#include "Fuzzer/SynthesisBuilder.h"
#include "Fuzzer/TestCase.h"
#include "Fuzzer/TestCaseSynthesiser.h"

namespace caf {

void TestCaseSynthesiser::Synthesis(const TestCase &tc) {
  _builder.EnterMainFunction();

  std::vector<SynthesisVariable> retVars;
  retVars.reserve(tc.GetFunctionCallsCount());

  for (const auto& call : tc) {
    auto receiver = SynthesisVariable::Empty();
    if (call.HasThis()) {
      receiver = _builder.SynthesisConstant(call.GetThis());
    }

    std::vector<SynthesisVariable> args;
    args.reserve(call.GetArgsCount());

    for (auto arg : call) {
      args.push_back(_builder.SynthesisConstant(arg));
    }

    const auto& functionName = _store.GetFunction(call.funcId()).name();
    retVars.push_back(_builder.SynthesisFunctionCall(functionName, receiver, args));
  }

  _builder.LeaveFunction();
}

std::string TestCaseSynthesiser::GetCode() const {
  return _builder.GetCode();
}

} // namespace caf
