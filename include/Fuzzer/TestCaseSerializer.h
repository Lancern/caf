#ifndef CAF_TEST_CASE_SERIALIZER_H
#define CAF_TEST_CASE_SERIALIZER_H

#include "Infrastructure/Intrinsic.h"
#include "Fuzzer/TestCase.h"

namespace caf {

/**
 * @brief Provide functions to serialize instances of `TestCase` to binary representation that can
 * be sent to the target program for fuzzing.
 *
 */
class TestCaseSerializer {
public:
  /**
   * @brief Write the given test case into the given output stream.
   *
   * @tparam Output the type of the output stream. This type should contain a `write` method that
   * can be called with the following signature:
   *
   * @code
   * void write(const uint8_t* buffer, size_t size);
   * @endcode
   *
   * @param o the output stream.
   * @param testCase the test case to be written.
   */
  template <typename Output>
  void Write(Output& o, const TestCase& testCase) const {
    // Write the number of function calls to the output stream.
    WriteTrivial(o, testCase.sequence().size());

    // Write each function call to the output stream.
    for (const auto& funcCall : testCase.sequence()) {
      Write(o, funcCall);
    }
  }

private:
  /**
   * @brief Write a value of a trivial type into the given output stream.
   *
   * @tparam Output The type of the output stream.
   * @tparam POD The type of the value. Trait std::is_trivial<ValueType> must be satisfied.
   * @param o the output stream.
   * @param value the value to be written.
   */
  template <typename Output, typename ValueType>
  void WriteTrivial(Output& o, ValueType value) const {
    static_assert(std::is_trivial<ValueType>::value,
        "Type argument ValueType is not a trivial type.");
    o.write(reinterpret_cast<const uint8_t *>(&value), sizeof(ValueType));
  }

  /**
   * @brief Write the given Value object into the given stream.
   *
   * @tparam Output type of the output stream.
   * @param o the output stream.
   * @param value the value to be written.
   */
  template <typename Output>
  void Write(Output& o, const Value& value) const {
    // Write kind of the value.
    WriteTrivial(o, value.kind());
    switch (value.kind()) {
      case ValueKind::BitsValue: {
        // Write the number of bytes followed by the raw binary data of the value to the output
        // stream.
        auto bitsValue = dynamic_cast<const BitsValue &>(value);
        WriteTrivial(o, bitsValue.size());
        o.write(bitsValue.data(), bitsValue.size());
        break;
      }
      case ValueKind::PointerValue: {
        // Write the underlying value pointed to by the pointer.
        auto pointerValue = dynamic_cast<const PointerValue &>(value);
        Write(o, *pointerValue.pointee());
        break;
      }
      case ValueKind::ArrayValue: {
        // Write the number of elements and each element into the output stream.
        auto arrayValue = dynamic_cast<const ArrayValue &>(value);
        WriteTrivial(o, arrayValue.size());
        for (auto el : arrayValue.elements()) {
          Write(o, *el);
        }
        break;
      }
      case ValueKind::StructValue: {
        // Write the ID of the activator and arguments to the activator to the output stream.
        auto structValue = dynamic_cast<const StructValue &>(value);
        WriteTrivial(o, structValue.activator()->id());
        WriteTrivial(o, structValue.args().size());
        for (auto arg : structValue.args()) {
          Write(o, *arg);
        }
        break;
      }
      default: CAF_UNREACHABLE;
    }
  }

  /**
   * @brief Write the given FunctionCall object into the given stream.
   *
   * @tparam Output the type of the stream.
   * @param o the output stream.
   * @param funcCall the function call object to be written.
   */
  template <typename Output>
  void Write(Output& o, const FunctionCall& funcCall) const {
    // Write function ID.
    WriteTrivial(o, funcCall.func()->id());

    // Write number of arguments.
    WriteTrivial(o, funcCall.args().size());

    // Write arguments.
    for (const auto& arg : funcCall.args()) {
      Write(o, *arg);
    }
  }
}; // class TestCaseSerializer

} // namespace caf

#endif
