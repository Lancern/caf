#ifndef CAF_TEST_CASE_H
#define CAF_TEST_CASE_H

#include "Fuzzer/FunctionCall.h"

#include <vector>

namespace caf {

/**
 * @brief Represent a test case, a.k.a. a sequence of function calls described
 * by FunctionCall instances.
 *
 */
class TestCase {
public:
  /**
   * @brief Construct a new TestCase object.
   *
   */
  explicit TestCase() noexcept
    : _sequence { }
  { }

  /**
   * @brief Add a function call to the test case.
   *
   */
  void AddFunctionCall(FunctionCall call) {
    _sequence.push_back(std::move(call));
  }

  /**
   * @brief Get the sequence of function calls carried out by the test case.
   *
   * @return const std::vector<FunctionCall> & the sequence of function calls carried out by the
   * test case.
   */
  const std::vector<FunctionCall>& calls() const noexcept { return _sequence; }

private:
  std::vector<FunctionCall> _sequence;
}; // class TestCase

} // namespace caf

#endif
