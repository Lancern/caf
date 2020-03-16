#ifndef CAF_TEST_CASE_MUTATOR_H
#define CAF_TEST_CASE_MUTATOR_H

#include "Infrastructure/Random.h"
#include "Fuzzer/TestCaseGenerator.h"

namespace caf {

class Corpus;
class TestCaseRef;
class TestCase;
class Value;

/**
 * @brief Test case mutator.
 *
 */
class TestCaseMutator {
public:
  using Options = TestCaseGenerator::Options;

  /**
   * @brief Construct a new TestCaseMutator object.
   *
   * @param corpus the test case corpus.
   * @param rnd the random number generator.
   */
  explicit TestCaseMutator(Corpus& corpus, Random<>& rnd)
    : _corpus(corpus),
      _rnd(rnd),
      _gen(corpus, rnd)
  { }

  TestCaseMutator(const TestCaseMutator &) = delete;
  TestCaseMutator(TestCaseMutator &&) noexcept = default;

  /**
   * @brief Get the options.
   *
   * @return Options& the options.
   */
  Options& options() { return _gen.options(); }

  /**
   * @brief Get the options.
   *
   * @return const Options& the options.
   */
  const Options& options() const { return _gen.options(); }

  /**
   * @brief Mutate the given test case.
   *
   * @param testCase the test case to mutate.
   * @return TestCaseRef the mutated test case.
   */
  TestCaseRef Mutate(TestCaseRef testCase);

private:
  Corpus& _corpus;
  Random<>& _rnd;
  TestCaseGenerator _gen;

  /**
   * @brief Mutate the given test case by adding a function call to the tail of the function call
   * sequence.
   *
   * @param testCase the test case to mutate.
   * @return TestCaseRef the mutated test case.
   */
  TestCaseRef AddFunctionCall(TestCaseRef testCase);

  /**
   * @brief Mutate the given test case by removing a function call from the function call sequence.
   *
   * @param testCase the test case to mutate.
   * @return TestCaseRef the mutated test case.
   */
  TestCaseRef RemoveFunctionCall(TestCaseRef testCase);

  /**
   * @brief Mutate the given test case by splicing it with a randomly chosen test case from the
   * corpus.
   *
   * @param testCase the test case to mutate.
   * @return TestCaseRef the mutated test case.
   */
  TestCaseRef Splice(TestCaseRef testCase);

  /**
   * @brief Mutate the given test case by choosing a function call and mutating `this` object of
   * the function call.
   *
   * @param testCase the test case to mutate.
   * @return TestCaseRef the mutated test case.
   */
  TestCaseRef MutateThis(TestCaseRef testCase);

  /**
   * @brief Mutate the given test case by choosing a function call and add an argument to the
   * function call.
   *
   * @param testCase the test case.
   * @return TestCaseRef the mutated test case.
   */
  TestCaseRef AddArgument(TestCaseRef testCase);

  /**
   * @brief Mutate the given test case by choosing a function call and remove an argument to the
   * function call.
   *
   * @param testCase test case.
   * @return TestCaseRef the mutated test case.
   */
  TestCaseRef RemoveArgument(TestCaseRef testCase);

  /**
   * @brief Mutate the given test case by choosing a function call and mutate an argument of it.
   *
   * @param testCase the test case.
   * @return TestCaseRef the mutated test case.
   */
  TestCaseRef MutateArgument(TestCaseRef testCase);

  /**
   * @brief Mutate the given value.
   *
   * @param value the value to mutate.
   * @param depth the current depth.
   * @return Value* the mutated value.
   */
  Value* Mutate(Value* value, int depth = 1);

  /**
   * @brief Mutate the given string value.
   *
   * @param value the string value to mutate.
   * @return StringValue* the mutated string value.
   */
  StringValue* MutateString(StringValue* value);

  /**
   * @brief Mutate the given string value by inserting a character into the string.
   *
   * @param value the string value to mutate.
   * @return StringValue* the mutated string value.
   */
  StringValue* InsertCharacter(StringValue* value);

  /**
   * @brief Mutate the given string value by removing a character from the string.
   *
   * @param value the string value to mutate.
   * @return StringValue* the mutated string value.
   */
  StringValue* RemoveCharacter(StringValue* value);

  /**
   * @brief Mutate the given string value by changing a character in the string.
   *
   * @param value the string value to mutate.
   * @return StringValue* the mutated string value.
   */
  StringValue* ChangeCharacter(StringValue* value);

  /**
   * @brief Mutate the given string value by exchanging two characters.
   *
   * @param value the string value to mutate.
   * @return StringValue* the mutated string value.
   */
  StringValue* ExchangeCharacters(StringValue* value);

  /**
   * @brief Mutate the given integer value.
   *
   * @param value the integer value to mutate.
   * @return IntegerValue* the mutated integer value.
   */
  IntegerValue* MutateInteger(IntegerValue* value);

  /**
   * @brief Mutate the given integer value by incrementing it with a random value.
   *
   * @param value the integer value to mutate.
   * @return IntegerValue* the mutated integer value.
   */
  IntegerValue* Increment(IntegerValue* value);

  /**
   * @brief Mutate the given integer value by negating its value.
   *
   * @param value the integer value to mutate.
   * @return IntegerValue* the mutated integer value.
   */
  IntegerValue* Negate(IntegerValue* value);

  /**
   * @brief Mutate the given integer value by performing AFL's bitflip operation.
   *
   * @param value the integer value to mutate.
   * @return IntegerValue* the mutated integer value.
   */
  IntegerValue* Bitflip(IntegerValue* value);

  /**
   * @brief Mutate the given floating point value.
   *
   * @param value the floating point value to mutate.
   * @return FloatValue* the mutated floating point value.
   */
  FloatValue* MutateFloat(FloatValue* value);

  /**
   * @brief Mutate the given floating point value by incrementing it with a random value.
   *
   * @param value the floating point value to mutate.
   * @return FloatValue* the mutated floating point value.
   */
  FloatValue* Increment(FloatValue* value);

  /**
   * @brief Mutate the given floating point value by negating it.
   *
   * @param value the floating point value to mutate.
   * @return FloatValue* the mutated floating point value.
   */
  FloatValue* Negate(FloatValue* value);

  /**
   * @brief Mutate the given array value.
   *
   * @param value the array value to mutate.
   * @param depth the current depth.
   * @return ArrayValue* the mutated array value.
   */
  ArrayValue* MutateArray(ArrayValue* value, int depth);

  /**
   * @brief Mutate the given array value by adding a new element to the back.
   *
   * @param value the array value to mutate.
   * @return ArrayValue* the mutated array value.
   */
  ArrayValue* PushElement(ArrayValue* value, int);

  /**
   * @brief Mutate the given array value by removing an element.
   *
   * @param value the array value to mutate.
   * @return ArrayValue* the mutated array value.
   */
  ArrayValue* RemoveElement(ArrayValue* value, int);

  /**
   * @brief Mutate the given array value by mutating an element.
   *
   * @param value the array value to mutate.
   * @param depth the current depth.
   * @return ArrayValue* the mutated array value.
   */
  ArrayValue* MutateElement(ArrayValue* value, int depth);

  /**
   * @brief Mutate the given array value by exchanging two elements.
   *
   * @param value the array value to mutate.
   * @return ArrayValue* the mutated array value.
   */
  ArrayValue* ExchangeElements(ArrayValue* value, int);
}; // class TestCaseMutator

} // namespace caf

#endif
