#ifndef CAF_SYNTHESIS_BUILDER_H
#define CAF_SYNTHESIS_BUILDER_H

#include "Infrastructure/Memory.h"

#include <cassert>
#include <cstddef>
#include <memory>
#include <string>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <vector>

namespace caf {

class Value;

/**
 * @brief Represent a synthesised variable.
 *
 */
class SynthesisVariable {
public:
  /**
   * @brief Construct an empty SynthesisVariable object.
   *
   */
  explicit SynthesisVariable()
    : _name()
  { }

  /**
   * @brief Construct a new SynthesisVariable object.
   *
   * @param name name of the variable.
   */
  explicit SynthesisVariable(std::string name)
    : _name(std::move(name))
  {
    assert(!_name.empty() && "name cannot be empty.");
  }

  /**
   * @brief Determine whether this object is empty.
   *
   * @return true if this object is empty.
   * @return false if this object is not empty.
   */
  bool IsEmpty() const { return _name.empty(); }

  /**
   * @brief Get the name of this variable.
   *
   * @return const std::string& name of this variable. Returns an empty string if this variable
   * is not bound to a name.
   */
  const std::string& GetName() const { return _name; }

private:
  std::string _name;
}; // class SynthesisVariable

/**
 * @brief Abstract base class for building synthesis.
 *
 */
class SynthesisBuilder {
public:
  SynthesisBuilder(const SynthesisBuilder &) = delete;
  SynthesisBuilder(SynthesisBuilder &&) = default;

  SynthesisBuilder& operator=(const SynthesisBuilder &) = delete;
  SynthesisBuilder& operator=(SynthesisBuilder &&) = default;

  virtual ~SynthesisBuilder() = default;

  /**
   * @brief Called when the synthesiser begins synthesising the main function.
   *
   */
  virtual void EnterMainFunction() { }

  /**
   * @brief Called when the synthesiser ends synthesising the main function.
   *
   */
  virtual void LeaveFunction() { }

  /**
   * @brief Synthesis a constant value.
   *
   * @param value the constant value.
   * @return SynthesisVariable& a SynthesisVariable object that can be used to refer to the
   * synthesised constant value.
   */
  SynthesisVariable SynthesisConstant(const Value* value);

  /**
   * @brief Synthesis a function call.
   *
   * @param functionName name of the function to be called.
   * @param isCtorCall true to make a constructor call rather a normal call.
   * @param receiver the receiver. This argument can be an empty SynthesisVariable object to
   * indicate that no special receiver is specified.
   * @param args arguments to the function call.
   * @return SynthesisVariable& a SynthesisVariable object that can be used to refer to the return
   * value of the function call.
   */
  virtual SynthesisVariable SynthesisFunctionCall(
      const std::string& functionName,
      bool isCtorCall,
      const SynthesisVariable& receiver,
      const std::vector<SynthesisVariable>& args);

  /**
   * @brief Get the synthesised code.
   *
   * @return std::string the synthesised code.
   */
  std::string GetCode() const;

protected:
  /**
   * @brief Construct a new SynthesisBuilder object.
   *
   */
  explicit SynthesisBuilder()
    : _variables(),
      _output(),
      _synthesisedVariables(),
      _funcRetVariables(),
      _varId(0)
  { }

  /**
   * @brief Construct a new SynthesisVariable object managed by this SynthesisBuilder object.
   *
   * @param name name of the variable.
   * @return SynthesisVariable& the SynthesisVariable object managed by this SynthesisBuilder.
   */
  SynthesisVariable CreateVariable(std::string name) {
    _variables.emplace_back(std::move(name));
    return _variables.back();
  }

  /**
   * @brief Get a std::ostringstream that wraps the output synthesis code.
   *
   * @return std::ostringstream& a std::ostringstream that wraps the output synthesis code.
   */
  std::ostringstream& GetOutput() { return _output; }

  /**
   * @brief Get a std::ostringstream that wraps the output synthesis code.
   *
   * @return const std::ostringstream& a std::ostringstream that wraps the output synthesis code.
   */
  const std::ostringstream& GetOutput() const { return _output; }

  /**
   * @brief Get the next variable name.
   *
   * @return std::string the next variable name.
   */
  std::string GetNextVariableName();

  /**
   * @brief Determine whether the given value has been synthesised.
   *
   * @param value the value.
   * @return true if the given value has been synthesised.
   * @return false if the given value has not been synthesised.
   */
  bool HasSynthesised(const Value* value) const {
    return _synthesisedVariables.find(value) != _synthesisedVariables.end();
  }

  /**
   * @brief Get the synthesised variable corresponding to the given value.
   *
   * @param value the value.
   * @return SynthesisVariable& the synthesised variable corresponding to the given value.
   */
  SynthesisVariable GetSynthesisedVariable(const Value* value) {
    auto variableIndex = _synthesisedVariables.at(value);
    return _variables.at(variableIndex);
  }

  /**
   * @brief Set the index of the corresponding synthesised variable of the given value.
   *
   * @param value the value.
   * @param variableIndex index of the corresponding synthesised variable.
   */
  void SetSynthesisedVariable(const Value* value, size_t variableIndex) {
    _synthesisedVariables[value] = variableIndex;
  }

  /**
   * @brief Set the value of the last synthesised variable.
   *
   * @param value the value of the last synthesised variable.
   */
  void SetLastSynthesisedValue(const Value* value) {
    auto index = static_cast<size_t>(_variables.size()) - 1;
    _synthesisedVariables[value] = index;
  }

  /**
   * @brief Get the variable holding the return value of the given function call.
   *
   * @param functionCallIndex the index of the function call.
   * @return SynthesisVariable the variable holding the return value.
   */
  SynthesisVariable GetRetValueVariable(size_t functionCallIndex) const {
    auto varIndex = _funcRetVariables.at(functionCallIndex);
    return _variables.at(varIndex);
  }

  /**
   * @brief When overridden in derived classes, write a variable reference expression to the
   * synthesised code that references the given variable in a language-specific manner.
   *
   * @param var the variable to reference.
   */
  virtual void WriteVariableRef(const std::string& varName);

  /**
   * @brief When overridden in derived classes, write a variable definition statement to the
   * synthesised code.
   *
   * @param varName the name of the variable.
   * @param value value of the variable.
   */
  virtual void WriteVariableDef(const std::string& varName, const Value* value) = 0;

  /**
   * @brief When overridden in derived classes, write a variable definition statement that
   * introduces a variable bound to an empty array.
   *
   * @param varName the name of the variable.
   */
  virtual void WriteEmptyArrayVariableDef(const std::string& varName) = 0;

  /**
   * @brief When overridden in derived classes, write an array push statement to the synthesised
   * code.
   *
   * @param varName name of the array variable.
   * @param elementVarName name of the variable referencing to the array element.
   */
  virtual void WriteArrayPushStatement(
      const std::string& varName,
      const std::string& elementVarName) = 0;

  /**
   * @brief When overridden in derived classes, write a function call statement to the synthesised
   * code.
   *
   * @param retVarName the name of the variable holding the return value.
   * @param functionName the name of the function to be called.
   * @param isCtorCall true to make a constructor call rather than a normal call.
   * @param receiverVarName name of the variable holding the receiver. This argument can be an empty
   * string to indicate that no special receiver is specified.
   * @param argVarNames names of the variables holding the arguments.
   */
  virtual void WriteFunctionCallStatement(
      const std::string& retVarName,
      const std::string& functionName,
      bool isCtorCall,
      const std::string& receiverVarName,
      const std::vector<std::string>& argVarNames) = 0;

private:
  std::vector<SynthesisVariable> _variables;
  std::ostringstream _output;
  std::unordered_map<const Value *, size_t> _synthesisedVariables;
  std::vector<size_t> _funcRetVariables;
  int _varId;
}; // class SynthesisBuilder

} // namespace caf

#endif
