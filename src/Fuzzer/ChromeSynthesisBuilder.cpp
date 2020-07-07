#include "Basic/CAFStore.h"
#include "Fuzzer/ChromeSynthesisBuilder.h"
#include "Fuzzer/SynthesisBuilder.h"
#include "Fuzzer/Value.h"
#include "CAFConfig.h"

#include <cctype>
#include <unordered_set>

namespace caf {

void ChromeSynthesisBuilder::EnterMainFunction() {
  auto& output = GetOutput();
  output << ".open netsec.ccert.edu.cn/chs/people/zengyishun\n";
}

void ChromeSynthesisBuilder::LeaveFunction() {
  auto& output = GetOutput();
  output << ";close();\n";
}

} // namespace caf
