#ifndef CAF_TEST_CASE_SERIALIZER_H
#define CAF_TEST_CASE_SERIALIZER_H

namespace caf {

class OutputStream;
class TestCase;
class FunctionCall;
class Value;

/**
 * @brief Serialize a test case into binary form.
 *
 */
class TestCaseSerializer {
public:
  /**
   * @brief Construct a new TestCaseSerializer object.
   *
   * @param corpus the corpus containing test cases.
   * @param out the output stream.
   */
  explicit TestCaseSerializer(OutputStream& out)
    : _out(out)
  { }

  TestCaseSerializer(const TestCaseSerializer &) = delete;
  TestCaseSerializer(TestCaseSerializer &&) noexcept = default;

  /**
   * @brief Serialize the given test case into binary form.
   *
   * @param testCase the test case to serialize.
   */
  void Serialize(const TestCase& testCase);

private:
  class SerializationContext;

  OutputStream& _out;

  /**
   * @brief Serialize the given function call into binary form.
   *
   * @param call the function call to serialize.
   * @param context the serialization context.
   */
  void Serialize(const FunctionCall& call, SerializationContext& context);

  /**
   * @brief Serialize the given value into binary form.
   *
   * @param value the value to serialize.
   * @param context the serialization context.
   */
  void Serialize(const Value* value, SerializationContext& context);
}; // class TestCaseSerializer

} // namespace caf

#endif
