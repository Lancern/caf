#ifndef CAF_VALUE_GENERATOR_H
#define CAF_VALUE_GENERATOR_H

#include "Infrastructure/Random.h"
#include "Fuzzer/Corpus.h"
#include "Fuzzer/FunctionCall.h"

namespace caf {

class Type;
class Value;
class BitsType;
class PointerType;
class ArrayType;
class StructType;
class BitsValue;
class PointerValue;
class FunctionPointerValue;
class ArrayValue;
class StructValue;
class PlaceholderValue;
class TestCase;

/**
 * @brief Generate test cases.
 *
 */
class TestCaseGenerator {
public:
  /**
   * @brief Construct a new Value Generator object
   *
   * @param corpus the test case corpus. New values will be generated into this corpus.
   * @param rnd the random number generator to use.
   */
  explicit TestCaseGenerator(CAFCorpus* corpus, Random<>& rnd)
    : _corpus(corpus),
      _rnd(rnd)
  { }

  /**
   * @brief Generate a new bits type value.
   *
   * @param type type of the bits type value to be generated.
   * @return BitsValue* the generated value.
   */
  BitsValue* GenerateNewBitsType(const BitsType* type);

  /**
   * @brief Generate a new pointer type value.
   *
   * @param type type of the pointer type value to be generated.
   * @return PointerValue* the generated value.
   */
  PointerValue* GenerateNewPointerType(const PointerType* type);

  /**
   * @brief Generate a new function pointer type value.
   *
   * @param type type of the function pointer.
   * @return FunctionPointerValue* the generated value.
   */
  FunctionPointerValue* GenerateNewFunctionPointerType(const PointerType* type);

  /**
   * @brief Generate a new array type value.
   *
   * @param type type of the array type value to be generated.
   * @return ArrayValue* the generated value.
   */
  ArrayValue* GenerateNewArrayType(const ArrayType* type);

  /**
   * @brief Generate a new struct type value.
   *
   * @param type type of the struct type value to be generated.
   * @return StructValue* the generated value.
   */
  StructValue* GenerateNewStructType(const StructType* type);

  /**
   * @brief Generate a new value of the given type.
   *
   * @param type type of the value to be generated.
   * @return Value* pointer to the generated new value.
   */
  Value* GenerateNewValue(const Type* type);

  /**
   * @brief Generate an argument of the given type.
   *
   * This function randomly applies the following strategies:
   * * Select an exiting value from the corresponding object pool in the corpus, if any;
   * * Create a new value and insert it into the corresponding object pool; any related values used
   * to construct the new value will be generated recursively until `BitsValue` is required. The
   * required `BitsValue` will be populated from the corresponding object pool directly.
   *
   * @param type the type of the argument.
   * @return Value* pointer to the generated `Value` instance wrapping around the argument.
   */
  Value* GenerateValue(const Type* type);

  /**
   * @brief Generate the argument at the given test case location.
   *
   * This function may also generate a placeholder referencing a previous function call return
   * value, with a specific probability.
   *
   * @param testCase the test case.
   * @param callIndex the index of the call.
   * @param argIndex the index of the argument of the specified call.
   * @return Value* the generated value.
   */
  Value* GenerateValueOrPlaceholder(const TestCase* testCase, size_t callIndex, size_t argIndex);

  /**
   * @brief Generate a function call. The callee is selected randomly within the `CAFStore` instance
   * in the corpus and the arguments are generated recursively.
   *
   * @return FunctionCall the generated function call.
   */
  FunctionCall GenerateCall();

  /**
   * @brief Generate a new test case.
   *
   * @param maxCalls the maximum number of calls to generate.
   * @return TestCase the generated test case.
   */
  CAFCorpusTestCaseRef GenerateTestCase(int maxCalls);

private:
  CAFCorpus* _corpus;
  Random<>& _rnd;
};

} // namespace caf

#endif
