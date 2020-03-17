#ifndef CAF_TEST_CASE_GENERATOR_H
#define CAF_TEST_CASE_GENERATOR_H

#include "Infrastructure/Random.h"
#include "Fuzzer/Value.h"

#include <cassert>
#include <memory>

namespace caf {

class Corpus;
class TestCase;
class TestCaseRef;
class FunctionCall;

/**
 * @brief Generate new test cases and values.
 *
 */
class TestCaseGenerator {
public:
  /**
   * @brief Options for the generator.
   *
   */
  struct Options {
    explicit Options()
      : MaxCalls(5),
        MaxStringLength(10),
        MaxArrayLength(5),
        MaxArguments(5),
        MaxDepth(3)
    { }

    size_t MaxCalls; // Maximum number of calls in generated test case.
    size_t MaxStringLength; // Maximum length of generated string values.
    size_t MaxArrayLength; // Maximum length of generated array values.
    size_t MaxArguments; // Maximum number of arguments to generate for a function call.
    size_t MaxDepth; // Maximum numbers of levels in the generated value.
  };

  /**
   * @brief Construct a new TestCaseGenerator object.
   *
   * @param corpus the corpus.
   * @param rnd the random number generator.
   */
  explicit TestCaseGenerator(Corpus& corpus, Random<>& rnd)
    : _corpus(corpus),
      _rnd(rnd),
      _opt()
  { }

  TestCaseGenerator(const TestCaseGenerator &) = delete;
  TestCaseGenerator(TestCaseGenerator &&) noexcept = default;

  /**
   * @brief Get the options.
   *
   * @return Options& the options.
   */
  Options& options() { return _opt; }

  /**
   * @brief Get the options.
   *
   * @return const Options& the options.
   */
  const Options& options() const { return _opt; }

  /**
   * @brief Generate a new test case.
   *
   * @return TestCaseRef the test case generated.
   */
  TestCaseRef GenerateTestCase();

  /**
   * @brief Generate a new function call.
   *
   * @return FunctionCall the function call generated.
   */
  FunctionCall GenerateFunctionCall();

  /**
   * @brief Generate a new value.
   *
   * @return Value* the value generated.
   */
  Value* GenerateValue() { return GenerateValue(1); }

  /**
   * @brief Generate a function value.
   *
   * @return FunctionValue* the generated function value.
   */
  FunctionValue* GenerateFunctionValue();

  /**
   * @brief Generate a char that can be added to a string value.
   *
   * @return char the generated character.
   */
  char GenerateStringCharacter();

private:
  Corpus& _corpus;
  Random<>& _rnd;
  Options _opt;

  /**
   * @brief Randomly generate a number indicating how many arguments should be generated for a
   * function call.
   *
   * @return size_t the number of arguments to generate.
   */
  size_t GenerateArgumentsCount();

  /**
   * @brief Generate a ValueKind value.
   *
   * @param generateArrayKind should we generate ArrayKind?
   * @return ValueKind the generated value.
   */
  ValueKind GenerateValueKind(bool generateArrayKind);

  /**
   * @brief Generate a new value.
   *
   * @param depth depth of the current genreation process.
   * @return Value* the generated value.
   */
  Value* GenerateValue(size_t depth);
}; // class TestCaseGenerator

} // namespace caf

#endif
