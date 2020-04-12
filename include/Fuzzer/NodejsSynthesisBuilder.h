#ifndef CAF_NODEJS_SYNTHESIS_BUILDER_H
#define CAF_NODEJS_SYNTHESIS_BUILDER_H

#include "Fuzzer/JavaScriptSynthesisBuilder.h"

#include <string>
#include <unordered_set>

namespace caf {

class CAFStore;

class NodejsSynthesisBuilder : public JavaScriptSynthesisBuilder {
public:
  explicit NodejsSynthesisBuilder(const CAFStore& store)
    : JavaScriptSynthesisBuilder { store },
      _imported()
  { }

  void EnterMainFunction() override;

protected:
  /**
   * @brief Write a require statement to the synthesised code.
   *
   * @param moduleName the name of the module to import.
   */
  void WriteRequireStatement(const std::string& moduleName);

  void WriteVariableDef(const std::string &varName, const Value *value) override;

  void WriteFunctionCallStatement(
      const std::string& retVarName,
      const std::string& functionName,
      bool isCtorCall,
      const std::string& receiverVarName,
      const std::vector<std::string>& argVarNames) override;

private:
  std::unordered_set<std::string> _imported;
}; // class NodejsSynthesisBuilder

} // namespace caf

#endif
