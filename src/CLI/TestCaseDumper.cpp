#include "TestCaseDumper.h"
#include "Basic/Function.h"
#include "Fuzzer/FunctionCall.h"
#include "Fuzzer/TestCase.h"

#include <iostream>

namespace caf {

void TestCaseDumper::Dump(const TestCase& tc) {
  for (auto callId = 0; callId < tc.GetFunctionCallCount(); ++callId) {
    const auto& call = tc.GetFunctionCall(callId);
    _printer.PrintWithColor(PrinterColor::BrightGreen, "CALL ");
    _printer << "#" << callId << ": "
             << call.func()->name()
             << Printer::endl;
  }
}

} // namespace caf
