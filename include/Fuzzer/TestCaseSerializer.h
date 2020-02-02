#ifndef CAF_TEST_CASE_SERIALIZER_H
#define CAF_TEST_CASE_SERIALIZER_H

#include "Infrastructure/Intrinsic.h"
#include "Infrastructure/Casting.h"
#include "Fuzzer/TestCase.h"
#include "Fuzzer/Value.h"
#include "Fuzzer/BitsValue.h"
#include "Fuzzer/PointerValue.h"
#include "Fuzzer/FunctionPointerValue.h"
#include "Fuzzer/ArrayValue.h"
#include "Fuzzer/StructValue.h"

#include <cstdint>

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
    WriteTrivial(o, testCase.calls().size());

    // Write each function call to the output stream.
    for (const auto& funcCall : testCase.calls()) {
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
        const auto& bitsValue = caf::dyn_cast<BitsValue>(value);
        WriteTrivial(o, bitsValue.size());
        o.write(bitsValue.data(), bitsValue.size());
        break;
      }
      case ValueKind::PointerValue: {
        // Write the underlying value pointed to by the pointer.
        const auto& pointerValue = caf::dyn_cast<PointerValue>(value);
        Write(o, *pointerValue.pointee());
        break;
      }
      case ValueKind::FunctionPointerValue: {
        // Write the ID of the pointee function.
        const auto& functionValue = caf::dyn_cast<FunctionPointerValue>(value);
        WriteTrivial(o, functionValue.functionId());
        break;
      }
      case ValueKind::ArrayValue: {
        // Write the number of elements and each element into the output stream.
        const auto& arrayValue = caf::dyn_cast<ArrayValue>(value);
        WriteTrivial(o, arrayValue.size());
        for (auto el : arrayValue.elements()) {
          Write(o, *el);
        }
        break;
      }
      case ValueKind::StructValue: {
        // Write the ID of the activator and arguments to the activator to the output stream.
        const auto& structValue = caf::dyn_cast<StructValue>(value);
        WriteTrivial(o, structValue.ctor()->id());
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
