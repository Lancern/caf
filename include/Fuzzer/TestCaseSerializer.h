#ifndef CAF_TEST_CASE_SERIALIZER_H
#define CAF_TEST_CASE_SERIALIZER_H

#include "Infrastructure/Intrinsic.h"
#include "Infrastructure/Casting.h"
#include "Infrastructure/Identity.h"
#include "Fuzzer/TestCase.h"
#include "Fuzzer/Value.h"
#include "Fuzzer/BitsValue.h"
#include "Fuzzer/PointerValue.h"
#include "Fuzzer/FunctionValue.h"
#include "Fuzzer/ArrayValue.h"
#include "Fuzzer/StructValue.h"
#include "Fuzzer/AggregateValue.h"
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
  bool HasValue(const Value* value) const;

  /**
   * @brief Add the given Value object to the context.
   *
   * @param value the Value object to be added.
   */
  void AddValue(const Value* value);

  /**
   * @brief Get the index of the already-exist Value object.
   *
   * If the given Value object does not exist in this context, the behavior is undefined.
   *
   * @param value the Value object.
   * @return size_t the index of the Value object.
   */
  size_t GetValueIndex(const Value* value) const;

  /**
   * @brief Skips the next available value index.
   *
   * After calling this function, the first new value will be added at the index i+2 where i is the
   * index of the last available value before calling this function.
   *
   */
  void SkipCurrentIndex();

  /**
   * @brief Set the adjusted index for the next test case placeholder index to the next available
   * index.
   *
   */
  void SetPlaceholderIndexAdjustment();

  /**
   * @brief Get the adjusted index for the given test case placeholder index.
   *
   * @param placeholderIndex the test case placeholder index.
   * @return size_t the adjustment of the given test case placeholder index.
   */
  size_t GetPlaceholderIndexAdjustment(size_t placeholderIndex) const;

private:
  std::unordered_map<const Value *, size_t> _valueIds;
  std::unordered_map<size_t, size_t> _adjustedPlaceholderIndexes;
  caf::IncrementIdAllocator<size_t> _valueIdAlloc;
  caf::IncrementIdAllocator<size_t> _placeholderIndexAlloc;
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
      Write(o, funcCall, context);

      // Adjust placeholder index that references to the return value of the current function.
      context.SetPlaceholderIndexAdjustment();
      // The next index is reserved for the return value of the current function.
      context.SkipCurrentIndex();
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
   * @param context the serialization context.
   * @param adjustPlaceholderIndex should we adjust the index of placeholder values?
   */
  template <typename Output>
  void Write(Output& o, const Value& value,
      details::TestCaseSerializationContext& context, bool adjustPlaceholderIndex) const {
    if (context.HasValue(&value)) {
      auto index = context.GetValueIndex(&value);
      PlaceholderValue ph { value.type(), index };
      Write(o, ph, context, false);
      return;
    }

    // Placeholder values does not reserve a valud index in the test case object pool.
    if (value.kind() != ValueKind::PlaceholderValue) {
      context.AddValue(&value);
    }

    // Write a boolean indicating whether the current value is a placeholder value.
    WriteInt<1>(o, static_cast<uint8_t>(value.kind() == ValueKind::PlaceholderValue));
    switch (value.kind()) {
      case ValueKind::BitsValue: {
        // Write the number of bytes followed by the raw binary data of the value to the output
        // stream.
        const auto& bitsValue = caf::dyn_cast<BitsValue>(value);
        assert(bitsValue.size() == caf::dyn_cast<BitsType>(value.type())->size() &&
            "BitsValue size does not match BitsType size.");
        WriteInt<4>(o, bitsValue.size());
        o.write(bitsValue.data(), bitsValue.size());
        break;
      }
      case ValueKind::PointerValue: {
        // Write the underlying value pointed to by the pointer.
        const auto& pointerValue = caf::dyn_cast<PointerValue>(value);
        Write(o, *pointerValue.pointee(), context, true);
        break;
      }
      case ValueKind::FunctionValue: {
        // Write the ID of the pointee function.
        const auto& functionValue = caf::dyn_cast<FunctionValue>(value);
        WriteInt<4>(o, functionValue.functionId());
        break;
      }
      case ValueKind::ArrayValue: {
        // Write each element into the output stream.
        const auto& arrayValue = caf::dyn_cast<ArrayValue>(value);
        assert(arrayValue.size() == caf::dyn_cast<ArrayType>(arrayValue.type())->size() &&
            "ArrayValue size does not match ArrayType size.");
        for (auto el : arrayValue.elements()) {
          Write(o, *el, context, true);
        }
        break;
      }
      case ValueKind::StructValue: {
        // Write the ID of the constructor and arguments to the constructor to the output stream.
        const auto& structValue = caf::dyn_cast<StructValue>(value);
        assert(structValue.args().size() == structValue.ctor()->GetArgCount() &&
            "Constructor arguments count does not match Constructor parameters count.");
        WriteInt<4>(o, structValue.ctor()->id());
        for (auto arg : structValue.args()) {
          Write(o, *arg, context, true);
        }
        break;
      }
      case ValueKind::AggregateValue: {
        // Write all the fields to the output stream.
        const auto& aggregateValue = caf::dyn_cast<AggregateValue>(value);
        assert(aggregateValue.GetFieldsCount() ==
              caf::dyn_cast<AggregateType>(aggregateValue.type())->GetFieldsCount() &&
            "AggregateValue fields count does not match AggregateType fields count.");
        for (auto field : aggregateValue.fields()) {
          Write(o, *field, context, true);
        }
        break;
      }
      case ValueKind::PlaceholderValue: {
        const auto& placeholderValue = caf::dyn_cast<PlaceholderValue>(value);
        auto index = placeholderValue.valueIndex();
        if (adjustPlaceholderIndex) {
          index = context.GetPlaceholderIndexAdjustment(index);
        }
        WriteInt<4>(o, index);
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
    assert(funcCall.args().size() == funcCall.func()->signature().GetArgCount() &&
        "Function arguments count does not match function parameters count.");

    // Write function ID.
    WriteInt<4>(o, funcCall.func()->id());

    // Write arguments.
    for (auto arg : funcCall.args()) {
      Write(o, *arg, context, true);
    }
  }
}; // class TestCaseSerializer

} // namespace caf

#endif
