#ifndef CAF_JAVASCRIPT_SYNTHESIS_BUILDER_H
#define CAF_JAVASCRIPT_SYNTHESIS_BUILDER_H

#include "Fuzzer/SynthesisBuilder.h"

namespace caf {

class CAFStore;

/**
 * @brief An implementation of SynthesisBuilder for JavaScript.
 *
 */
class JavaScriptSynthesisBuilder : public SynthesisBuilder {
public:
  /**
   * @brief Construct a new JavaScriptSynthesisBuilder object.
   *
   * @param store the CAF store.
   */
  explicit JavaScriptSynthesisBuilder(const CAFStore& store)
    : _store(store)
  { }

  /**
   * @brief Get the CAF metadata store.
   *
   * @return const CAFStore& CAF metadata store.
   */
  const CAFStore& store() const { return _store; }

protected:
  /**
   * @brief Write a literal value to the synthesised code.
   *
   * @param value the value.
   */
  virtual void WriteLiteralValue(const Value* value);

  void WriteVariableDef(const std::string &varName, const Value *value) override;

  void WriteEmptyArrayVariableDef(const std::string &varName) override;

  void WriteArrayPushStatement(
      const std::string &varName,
      const std::string &elementVarName) override;

  void WriteFunctionCallStatement(
      const std::string& retVarName,
      const std::string& functionName,
      bool isCtorCall,
      const std::string& receiverVarName,
      const std::vector<std::string>& argVarNames) override;

  std::string EscapeString(const std::string& s) const;

private:
  const CAFStore& _store;
}; // class JavaScriptSynthesisBuilder

} // namespace caf

#endif
