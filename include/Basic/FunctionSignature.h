#ifndef CAF_FUNCTION_SIGNATURE_H
#define CAF_FUNCTION_SIGNATURE_H

#include "Basic/ValueKind.h"

#include <cstdint>
#include <vector>
#include <initializer_list>

namespace caf {

/**
 * @brief A lightweight type that represents a set of ValueKind values.
 *
 */
class ValueKindSet {
public:
  /**
   * @brief Construct a new empty ValueKindSet object.
   *
   */
  explicit ValueKindSet()
    : _raw(0)
  { }

  /**
   * @brief Construct a new ValueKindSet object.
   *
   * @param kind The only ValueKind value in the constructed ValueKindSet object.
   */
  explicit ValueKindSet(ValueKind kind)
    : _raw(ToRaw(kind))
  { }

  /**
   * @brief Construct a new ValueKindSet object.
   *
   * @param kinds The initial ValueKind values in the constructed ValueKindSet object.
   */
  explicit ValueKindSet(std::initializer_list<ValueKind> kinds)
    : _raw(0) {
    for (auto kind : kinds) {
      Add(kind);
    }
  }

  /**
   * @brief Determine whether the given ValueKind valud is present in the current set.
   *
   * @param kind The value to determine.
   * @return true if the given value is present in the current set.
   * @return false if the given value is not present in the current set.
   */
  bool Has(ValueKind kind) const {
    return (_raw & ToRaw(kind)) != 0;
  }

  /**
   * @brief Add the given value to the current set.
   *
   * @param kind the value to add.
   */
  void Add(ValueKind kind) {
    _raw |= ToRaw(kind);
  }

  /**
   * @brief Remove the given value from the current set.
   *
   * @param kind the value to remove.
   */
  void Remove(ValueKind kind) {
    _raw &= ~ToRaw(kind);
  }

  /**
   * @brief Create a ValueKindSet that contains all value kinds.
   *
   * @return ValueKindSet the created ValueKindSet object.
   */
  static ValueKindSet CreateFull() {
    ValueKindSet set {
#define ECHO_VALUE_KIND(kind) ValueKind::kind,
      CAF_VALUE_KIND_LIST(ECHO_VALUE_KIND)
#undef ECHO_VALUE_KIND
    };
    set.Remove(ValueKind::Placeholder);
    return set;
  }

private:
  static uint8_t ToRaw(ValueKind kind) {
    return 1 << static_cast<uint8_t>(kind);
  }

  uint8_t _raw;
}; // class ValueKindSet

/**
 * @brief A function signature.
 *
 */
class FunctionSignature {
public:
  /**
   * @brief Construct a new FunctionSignature object.
   *
   */
  explicit FunctionSignature() = default;

  FunctionSignature(const FunctionSignature &) = delete;
  FunctionSignature(FunctionSignature &&) noexcept = default;

  FunctionSignature& operator=(const FunctionSignature &) = delete;
  FunctionSignature& operator=(FunctionSignature &&) = default;

  /**
   * @brief Get all valid types of `this` implicit parameter.
   *
   * @return ValueKindSet all valid types of `this` implicit parameter.
   */
  ValueKindSet GetThisKinds() const { return _thisKinds; }

  /**
   * @brief Get all valid types of `this` implicit parameter.
   *
   * @return ValueKindSet& all valid types of `this` implicit parameter.
   */
  ValueKindSet& GetThisKinds() { return _thisKinds; }

  /**
   * @brief Set valid types of `this` implicit parameter.
   *
   * @param kinds all valid types of `this` implicit parameter.
   */
  void SetThisKinds(ValueKindSet kinds) { _thisKinds = kinds; }

  /**
   * @brief Get all valid types of all parameters.
   *
   * @return const std::vector<ValueKindSet>& all valid types of all parameters.
   */
  const std::vector<ValueKindSet>& GetParamKinds() const { return _paramKinds; }

  /**
   * @brief Add a parameter which can be any of the given types.
   *
   * @param kinds the allowed types of the new parameter.
   */
  void AddParamKinds(ValueKindSet kinds) {
    _paramKinds.push_back(kinds);
  }

  /**
   * @brief Get all valid types of the specified parameter.
   *
   * @param index index of the parameter.
   * @return ValueKindSet all valid types of the specified parameter.
   */
  ValueKindSet GetParamValueKinds(int index) const { return _paramKinds.at(index); }

  /**
   * @brief Get all valid types of the specified parameter.
   *
   * @param index index of the parameter.
   * @return ValueKindSet all valid types of the specified parameter.
   */
  ValueKindSet& GetParamValueKinds(int index) { return _paramKinds.at(index); }

private:
  ValueKindSet _thisKinds;
  std::vector<ValueKindSet> _paramKinds;
}; // class FunctionSignature

} // namespace caf

#endif
