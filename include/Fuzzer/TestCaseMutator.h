#ifndef CAF_TEST_CASE_MUTATOR_H
#define CAF_TEST_CASE_MUTATOR_H

#include "Infrastructure/Intrinsic.h"
#include "Infrastructure/Random.h"
#include "Fuzzer/Corpus.h"
#include "Fuzzer/TestCaseGenerator.h"

namespace caf {

class BitsValue;
class PointerValue;
class FunctionPointerValue;
class ArrayValue;
class StructValue;
class PlaceholderValue;

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

private:
  class MutationContext;

  CAFCorpus* _corpus;
  Random<>& _rnd;
  TestCaseGenerator _valueGen;

  CAFCorpusTestCaseRef Splice(MutationContext& context);

  CAFCorpusTestCaseRef InsertCall(MutationContext& context);

  CAFCorpusTestCaseRef RemoveCall(MutationContext& context);

  CAFCorpusTestCaseRef MutateSequence(MutationContext& context);

  void FixPlaceholderValuesAfterSequenceMutation(CAFCorpusTestCaseRef tc);

  void FlipBits(uint8_t* buffer, size_t size, size_t width);

  void FlipBytes(uint8_t* buffer, size_t size, size_t width);

  void Arith(uint8_t* buffer, size_t size, size_t width);

  Value* MutateBitsValue(const BitsValue* value);

  Value* MutatePointerValue(const PointerValue* value, MutationContext& context);

  Value* MutateFunctionPointerValue(const FunctionPointerValue* value);

  Value* MutateArrayValue(const ArrayValue* value, MutationContext& context);

  Value* MutateStructValue(const StructValue* value, MutationContext& context);

  Value* MutatePlaceholderValue(const PlaceholderValue* value, MutationContext& context);

  /**
   * @brief Mutate the given value.
   *
   * @param value the value to be mutated.
   * @param context the mutation context.
   * @return Value* pointer to the mutated value.
   */
  Value* MutateValue(const Value* value, MutationContext& context);

  CAFCorpusTestCaseRef MutateArgument(MutationContext& context);
}; // class TestCaseMutator


} // namespace caf

#endif
