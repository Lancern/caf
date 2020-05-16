#ifndef CAF_CHROME_SYNTHESIS_BUILDER_H
#define CAF_CHROME_SYNTHESIS_BUILDER_H

#include "Fuzzer/JavaScriptSynthesisBuilder.h"
#include "Fuzzer/SynthesisBuilder.h"

#include <string>
#include <unordered_set>

namespace caf {

class CAFStore;

class ChromeSynthesisBuilder : public JavaScriptSynthesisBuilder {
public:
  explicit ChromeSynthesisBuilder(const CAFStore& store)
    : JavaScriptSynthesisBuilder { store },
    apiNum(0)
  { }

  SynthesisVariable SynthesisFunctionCall(
      const std::string& functionName,
      bool isCtorCall,
      const SynthesisVariable& receiver,
      const std::vector<SynthesisVariable>& args);
  
  void EnterMainFunction() override;

  void LeaveFunction() override;

protected:
  int32_t apiNum;

}; // class ChromeSynthesisBuilder

} // namespace caf

#endif
