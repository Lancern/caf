#ifndef CAF_TEST_CASE_DESERIALIZER_H
#define CAF_TEST_CASE_DESERIALIZER_H

#include "Fuzzer/TestCase.h"

namespace caf {

class InputStream;
class ObjectPool;
class FunctionCall;
class Value;

/**
 * @brief Deserialize test cases frrom binary form.
 *
 */
class TestCaseDeserializer {
public:
  /**
   * @brief Construct a new TestCaseDeserializer object.
   *
   * @param pool the object pool.
   * @param in the input stream.
   */
  explicit TestCaseDeserializer(ObjectPool& pool, InputStream& in)
    : _pool(pool), _in(in)
  { }

  TestCaseDeserializer(const TestCaseDeserializer &) = delete;
  TestCaseDeserializer(TestCaseDeserializer &&) noexcept = default;

  /**
   * @brief Deserialize a test case from the underlying stream.
   *
   * @return TestCase the test case deserialized.
   */
  TestCase Deserialize();

private:
  class DeserializationContext;

  ObjectPool& _pool;
  InputStream& _in;

  /**
   * @brief Deserialize a function call from the underlying stream.
   *
   * @param context the deserialization context.
   * @return FunctionCall the deserialized function call.
   */
  FunctionCall DeserializeFunctionCall(DeserializationContext& context);

  /**
   * @brief Deserialize a value from the underlying stream.
   *
   * @param context the deserialization context.
   * @return Value* the deserialized value.
   */
  Value* DeserializeValue(DeserializationContext& context);
}; // class TestCaseDeserializer

} // namespace caf

#endif
