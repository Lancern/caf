#ifndef CAF_TEST_CASE_H
#define CAF_TEST_CASE_H

#include "Fuzzer/FunctionCall.h"

#include <vector>

namespace caf {

class Value;

/**
 * @brief Represent a test case, a.k.a. a sequence of API function calls and a pool of objects used
 * as arguments and return values of these function calls.
 *
 */
class TestCase {
public:
  /**
   * @brief Add a function call to the test case.
   *
   * @param call the API function call to add.
   */
  void AddFunctionCall(FunctionCall call);

  /**
   * @brief Get the function call at the given index.
   *
   * @param index the index of the function call.
   * @return FunctionCall& the function call at the given index.
   */
  FunctionCall& GetFunctionCall(size_t index) { return _calls[index]; }

  /**
   * @brief Get the function call at the given index.
   *
   * @param index the index of the function call.
   * @return const FunctionCall& the function call at the given index.
   */
  const FunctionCall& GetFunctionCall(size_t index) const { return _calls[index]; }

  /**
   * @brief Get the sequence of function calls carried out by the test case.
   *
   * @return const std::vector<FunctionCall> & the sequence of function calls carried out by the
   * test case.
   */
  const std::vector<FunctionCall>& calls() const { return _calls; }

private:
  std::vector<FunctionCall> _calls;
}; // class TestCase

} // namespace caf

#endif
