#ifndef CAF_TEST_CASE_DESERIALIZER_H
#define CAF_TEST_CASE_DESERIALIZER_H

#include "Fuzzer/Corpus.h"

namespace caf {

class InputStream;
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
   * @param corpus
   * @param in
   */
  explicit TestCaseDeserializer(Corpus& corpus, InputStream& in)
    : _corpus(corpus), _in(in)
  { }

  TestCaseDeserializer(const TestCaseDeserializer &) = delete;
  TestCaseDeserializer(TestCaseDeserializer &&) noexcept = default;

  /**
   * @brief Deserialize a test case from the underlying stream.
   *
   * @return TestCaseRef the test case deserialized.
   */
  TestCaseRef Deserialize();

private:
  class DeserializationContext;

  Corpus& _corpus;
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
