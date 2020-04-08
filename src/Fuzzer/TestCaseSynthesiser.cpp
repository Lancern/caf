#include "Basic/CAFStore.h"
#include "Fuzzer/SynthesisBuilder.h"
#include "Fuzzer/TestCase.h"
#include "Fuzzer/TestCaseSynthesiser.h"

namespace caf {

void TestCaseSynthesiser::Synthesis(const TestCase &tc) {
  _builder.EnterMainFunction();

  for (const auto& call : tc) {
    SynthesisVariable receiver { };
    if (call.HasThis()) {
      receiver = _builder.SynthesisConstant(call.GetThis());
    }

    std::vector<SynthesisVariable> args;
    args.reserve(call.GetArgsCount());

    for (auto arg : call) {
      args.push_back(_builder.SynthesisConstant(arg));
    }

    const auto& functionName = _store.GetFunction(call.funcId()).name();
    _builder.SynthesisFunctionCall(functionName, call.IsConstructorCall(), receiver, args);
  }

  _builder.LeaveFunction();
}

std::string TestCaseSynthesiser::GetCode() const {
  return _builder.GetCode();
}

} // namespace caf
