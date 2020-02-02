#ifndef CAF_TEST_CASE_MUTATOR_H
#define CAF_TEST_CASE_MUTATOR_H

#include "Infrastructure/Intrinsic.h"
#include "Infrastructure/Random.h"
#include "Fuzzer/Corpus.h"
#include "Fuzzer/ValueGenerator.h"

namespace caf {

class BitsValue;
class PointerValue;
class FunctionPointerValue;
class ArrayValue;
class StructValue;

/**
 * @brief Provide mutation strategy for test cases.
 *
 */
class TestCaseMutator {
public:
  /**
   * @brief Construct a new TestCaseMutator object.
   *
   * @param corpus test corpus.
   * @param rnd random number generator to use.
   */
  explicit TestCaseMutator(CAFCorpus* corpus, Random<>& rnd)
    : _corpus(corpus),
      _rnd(rnd),
      _valueGen { corpus, rnd }
  { }

  /**
   * @brief Get the corpus.
   *
   * @return CAFCorpus* pointer to the corpus.
   */
  CAFCorpus* corpus() const { return _corpus; }

  /**
   * @brief Mutate the given test case and produce a new one.
   *
   * @param corpus the corpus of test cases.
   * @return TestCase the mutated test case.
   */
  CAFCorpusTestCaseRef Mutate(CAFCorpusTestCaseRef testCase);

  /**
   * @brief Mutate the given value.
   *
   * @param value the value to be mutated.
   * @return Value* pointer to the mutated value.
   */
  Value* MutateValue(const Value* value);

private:
  CAFCorpus* _corpus;
  Random<>& _rnd;
  ValueGenerator _valueGen;

  CAFCorpusTestCaseRef Splice(CAFCorpusTestCaseRef previous);

  CAFCorpusTestCaseRef InsertCall(CAFCorpusTestCaseRef previous);

  CAFCorpusTestCaseRef RemoveCall(CAFCorpusTestCaseRef previous);

  CAFCorpusTestCaseRef MutateSequence(CAFCorpusTestCaseRef previous);

  void FlipBits(uint8_t* buffer, size_t size, size_t width);

  void FlipBytes(uint8_t* buffer, size_t size, size_t width);

  void Arith(uint8_t* buffer, size_t size, size_t width);

  Value* MutateBitsValue(const BitsValue* value);

  Value* MutatePointerValue(const PointerValue* value);

  Value* MutateFunctionPointerValue(const FunctionPointerValue* value);

  Value* MutateArrayValue(const ArrayValue* value);

  Value* MutateStructValue(const StructValue* value);

  CAFCorpusTestCaseRef MutateArgument(CAFCorpusTestCaseRef previous);
}; // class TestCaseMutator


} // namespace caf

#endif
