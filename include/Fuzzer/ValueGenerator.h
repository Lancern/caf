#ifndef CAF_VALUE_GENERATOR_H
#define CAF_VALUE_GENERATOR_H

#include "Infrastructure/Random.h"
#include "Fuzzer/FunctionCall.h"

namespace caf {

class CAFCorpus;
class Type;
class Value;
class BitsType;
class PointerType;
class ArrayType;
class StructType;

/**
 * @brief Generate test case values.
 *
 */
class ValueGenerator {
public:
  /**
   * @brief Construct a new Value Generator object
   *
   * @param corpus the test case corpus. New values will be generated into this corpus.
   * @param rnd the random number generator to use.
   */
  explicit ValueGenerator(CAFCorpus* corpus, Random<>& rnd)
    : _corpus(corpus),
      _rnd(rnd)
  { }

  /**
   * @brief Generate a new bits type value.
   *
   * @param type type of the bits type value to be generated.
   * @return Value* the generated value.
   */
  Value* GenerateNewBitsType(const BitsType* type);

  /**
   * @brief Generate a new pointer type value.
   *
   * @param type type of the pointer type value to be generated.
   * @return Value* the generated value.
   */
  Value* GenerateNewPointerType(const PointerType* type);

  /**
   * @brief Generate a new function pointer type value.
   *
   * @param type type of the function pointer.
   * @return Value* the generated value.
   */
  Value* GenerateNewFunctionPointerType(const PointerType* type);

  /**
   * @brief Generate a new array type value.
   *
   * @param type type of the array type value to be generated.
   * @return Value* the generated value.
   */
  Value* GenerateNewArrayType(const ArrayType* type);

  /**
   * @brief Generate a new struct type value.
   *
   * @param type type of the struct type value to be generated.
   * @return Value* the generated value.
   */
  Value* GenerateNewStructType(const StructType* type);

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
   * @brief Generate a function call. The callee is selected randomly within the `CAFStore` instance
   * in the corpus and the arguments are generated recursively.
   *
   * @return FunctionCall the generated function call.
   */
  FunctionCall GenerateCall();

private:
  CAFCorpus* _corpus;
  Random<>& _rnd;
};

} // namespace caf

#endif
