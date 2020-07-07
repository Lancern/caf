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
    : JavaScriptSynthesisBuilder { store }
  { }
  
  void EnterMainFunction() override;

  void LeaveFunction() override;

}; // class ChromeSynthesisBuilder

} // namespace caf

#endif
