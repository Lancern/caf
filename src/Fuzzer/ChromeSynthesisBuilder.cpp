#include "Basic/CAFStore.h"
#include "Fuzzer/ChromeSynthesisBuilder.h"
#include "Fuzzer/SynthesisBuilder.h"
#include "Fuzzer/Value.h"
#include "CAFConfig.h"

#include <cctype>
#include <unordered_set>

namespace caf {

SynthesisVariable ChromeSynthesisBuilder::SynthesisFunctionCall(
    const std::string& functionName,
    bool isCtorCall,
    const SynthesisVariable& receiver,
    const std::vector<SynthesisVariable>& args) {

  SynthesisBuilder::SynthesisFunctionCall(
    functionName,
    isCtorCall,
    receiver,
    args
  );
  
  auto& output = GetOutput();
  output << "console.log(\"API No." + std::to_string(apiNum++) + " finished.\");";
}

void ChromeSynthesisBuilder::EnterMainFunction() {
  auto& output = GetOutput();
  output << ".open www.baidu.com\n";
}

void ChromeSynthesisBuilder::LeaveFunction() {
  auto& output = GetOutput();
  output << "; close();";
}

} // namespace caf
