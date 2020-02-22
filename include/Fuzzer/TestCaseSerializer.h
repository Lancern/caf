#ifndef CAF_TEST_CASE_SERIALIZER_H
#define CAF_TEST_CASE_SERIALIZER_H

#include "Infrastructure/Intrinsic.h"
#include "Infrastructure/Casting.h"
#include "Infrastructure/Identity.h"
#include "Fuzzer/TestCase.h"
#include "Fuzzer/Value.h"
#include "Fuzzer/BitsValue.h"
#include "Fuzzer/PointerValue.h"
#include "Fuzzer/FunctionPointerValue.h"
#include "Fuzzer/ArrayValue.h"
#include "Fuzzer/StructValue.h"
#include "Fuzzer/PlaceholderValue.h"

#include <cstddef>
#include <cstdint>

#include <vector>
#include <unordered_map>

namespace caf {

namespace details {

/**
 * @brief Provide context information while serializing TestCase objects.
 *
 */
class TestCaseSerializationContext {
public:
  explicit TestCaseSerializationContext() = default;

  TestCaseSerializationContext(const TestCaseSerializationContext &) = delete;
  TestCaseSerializationContext(TestCaseSerializationContext &&) noexcept = default;

  TestCaseSerializationContext& operator=(const TestCaseSerializationContext &) = delete;
  TestCaseSerializationContext& operator=(TestCaseSerializationContext &&) = default;

  /**
   * @brief Determine whether the given Value object is contained in this context.
   *
   * @param value the Value object.
   * @return true if the given Value object is contained in this context.
   * @return false if the given Value object is not contained in this context.
   */
  bool HasValue(Value* value) const;

  /**
   * @brief Add the given Value object to the context.
   *
   * @param value the Value object to be added.
   */
  void AddValue(Value* value);

  /**
   * @brief Get the index of the already-exist Value object.
   *
   * If the given Value object does not exist in this context, the behavior is undefined.
   *
   * @param value the Value object.
   * @return size_t the index of the Value object.
   */
  size_t GetValueIndex(Value* value) const;

  /**
   * @brief Skips the next available value index.
   *
   * After calling this function, the first new value will be added at the index i+2 where i is the
   * index of the last available value before calling this function.
   *
   */
  void SkipCurrentIndex();

private:
  std::unordered_map<Value *, size_t> _valueIds;
  caf::IncrementIdAllocator<size_t> _valueIdAlloc;
}; // class TestCaseSerializationContext

}; // namespace details

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
   * void write(const void* buffer, size_t size);
   * @endcode
   *
   * @param o the output stream.
   * @param testCase the test case to be written.
   */
  template <typename Output>
  void Write(Output& o, const TestCase& testCase) const {
    details::TestCaseSerializationContext context;

    // Write the number of function calls to the output stream.
    WriteInt<4>(o, testCase.calls().size());

    // Write each function call to the output stream.
    for (const auto& funcCall : testCase.calls()) {
      // Skip current value index because this index is reserved for the return value of the
      // current function.
      context.SkipCurrentIndex();
      Write(o, funcCall, context);
    }
  }

private:
  /**
   * @brief Write a value of an integral type into the given output stream.
   *
   * @tparam ValueSize the size of the value to be written into the given stream.
   * @tparam Output The type of the output stream.
   * @tparam ValueType The type of the value. Trait std::is_integral<ValueType> must be satisfied.
   * @param o the output stream.
   * @param value the value to be written.
   */
  template <size_t ValueSize, typename Output, typename ValueType>
  void WriteInt(Output& o, ValueType value) const {
    static_assert(std::is_integral<ValueType>::value,
        "Type argument ValueType is not a trivial type.");
    auto casted = int_cast<ValueSize>(value);
    o.write(reinterpret_cast<void *>(&casted), sizeof(casted));
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
    WriteInt<4>(o, static_cast<int>(value.kind()));
    switch (value.kind()) {
      case ValueKind::BitsValue: {
        // Write the number of bytes followed by the raw binary data of the value to the output
        // stream.
        const auto& bitsValue = caf::dyn_cast<BitsValue>(value);
        WriteInt<4>(o, bitsValue.size());
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
        WriteInt<4>(o, functionValue.functionId());
        break;
      }
      case ValueKind::ArrayValue: {
        // Write the number of elements and each element into the output stream.
        const auto& arrayValue = caf::dyn_cast<ArrayValue>(value);
        WriteInt<4>(o, arrayValue.size());
        for (auto el : arrayValue.elements()) {
          Write(o, *el);
        }
        break;
      }
      case ValueKind::StructValue: {
        // Write the ID of the activator and arguments to the activator to the output stream.
        const auto& structValue = caf::dyn_cast<StructValue>(value);
        WriteInt<4>(o, structValue.ctor()->id());
        for (auto arg : structValue.args()) {
          Write(o, *arg);
        }
        break;
      }
      case ValueKind::PlaceholderValue: {
        const auto& placeholderValue = caf::dyn_cast<PlaceholderValue>(value);
        WriteInt<4>(o, placeholderValue.valueIndex());
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
   * @param context the context information during serialization.
   */
  template <typename Output>
  void Write(Output& o, const FunctionCall& funcCall,
      details::TestCaseSerializationContext& context) const {
    // Write function ID.
    WriteInt<4>(o, funcCall.func()->id());

    // Write arguments.
    for (auto arg : funcCall.args()) {
      if (context.HasValue(arg)) {
        // This argument has already been serialized into the underlying stream.
        // Use a PlaceholderValue referencing that object instead to reduce data size.
        PlaceholderValue ph { arg->type(), context.GetValueIndex(arg) };
        Write(o, ph);
        context.SkipCurrentIndex();
      } else {
        Write(o, *arg);
        context.AddValue(arg);
      }
    }
  }
}; // class TestCaseSerializer

} // namespace caf

#endif
