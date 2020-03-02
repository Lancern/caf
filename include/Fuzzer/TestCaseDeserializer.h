#ifndef CAF_TEST_CASE_DESERIALIZER_H
#define CAF_TEST_CASE_DESERIALIZER_H

#include "Infrastructure/Casting.h"
#include "Infrastructure/Identity.h"
#include "Infrastructure/Intrinsic.h"
#include "Basic/CAFStore.h"
#include "Basic/Function.h"
#include "Basic/Type.h"
#include "Basic/BitsType.h"
#include "Basic/PointerType.h"
#include "Basic/ArrayType.h"
#include "Basic/StructType.h"
#include "Basic/AggregateType.h"
#include "Basic/Constructor.h"
#include "Fuzzer/Corpus.h"
#include "Fuzzer/ObjectPool.h"
#include "Fuzzer/FunctionCall.h"
#include "Fuzzer/Value.h"
#include "Fuzzer/BitsValue.h"
#include "Fuzzer/PointerValue.h"
#include "Fuzzer/FunctionPointerValue.h"
#include "Fuzzer/ArrayValue.h"
#include "Fuzzer/StructValue.h"
#include "Fuzzer/AggregateValue.h"
#include "Fuzzer/PlaceholderValue.h"

#include <cstdint>
#include <utility>
#include <type_traits>
#include <vector>
#include <unordered_map>

namespace caf {

namespace details {

/**
 * @brief Provide context information when deserializing test cases.
 *
 */
class TestCaseDeserializationContext {
public:
  /**
   * @brief Construct a new Test Case Deserialization Context object.
   *
   */
  explicit TestCaseDeserializationContext() = default;

  TestCaseDeserializationContext(const TestCaseDeserializationContext &) = delete;
  TestCaseDeserializationContext(TestCaseDeserializationContext &&) noexcept = default;

  TestCaseDeserializationContext& operator=(const TestCaseDeserializationContext &) = delete;
  TestCaseDeserializationContext& operator=(TestCaseDeserializationContext &&) = default;

  /**
   * @brief Allocate a new test case object pool index.
   *
   * @return size_t the allocated test case object pool index.
   */
  size_t AllocValueIndex();

  /**
   * @brief Set the value at the given test case object pool index.
   *
   * @param index the test case object pool index.
   * @param value the value.
   */
  void SetValue(size_t index, Value* value);

  /**
   * @brief Get the value at the given test case object pool index.
   *
   * This function will trigger an assertion failure if the given index is out of range.
   *
   * @param index the test case object pool index.
   * @return Value* the value at the given index.
   */
  Value* GetValue(size_t index) const;

  /**
   * @brief Skip the next test case object pool index. The value at the next test case object pool
   * index will be nullptr.
   *
   */
  void SkipNextValueIndex();

  /**
   * @brief Skip the next test case placeholder index.
   *
   */
  void SkipNextPlaceholderIndex();

  /**
   * @brief Associate the next test case placeholder index with the next test case object pool value
   * index.
   *
   * After calling this function, the return value of IsPlaceholderIndex given the next available
   * test case object pool index will be true, and the return value of GetPlaceholderIndex given the
   * next available test case object pool index will be the next available test case placeholder
   * index.
   *
   */
  void ReservePlaceholderIndex();

  /**
   * @brief Determine whether the given test case object pool index is associated with a test case
   * placeholder index.
   *
   * @param objectPoolIndex the test case object pool index.
   * @return true if the given test case object pool index is a placeholder index.
   * @return false if the given test case object pool index is not a placeholder index.
   */
  bool IsPlaceholderIndex(size_t objectPoolIndex) const;

  /**
   * @brief Get the placeholder index associated with the given test case object pool index.
   *
   * @param objectPoolIndex the test case object pool index.
   * @return size_t the placeholder index associated with the given test case object pool index.
   */
  size_t GetPlaceholderIndex(size_t objectPoolIndex) const;

private:
  std::vector<Value *> _values;
  std::unordered_map<size_t, size_t> _placeholderIndexAdjustment;
  caf::IncrementIdAllocator<size_t> _placeholderIndexAlloc;
}; // class TestCaseDeserializationContext

} // namespace details

/**
 * @brief Provide functions to deserialize instances of TestCase from binary representations.
 *
 */
class TestCaseDeserializer {
public:
  explicit TestCaseDeserializer(CAFCorpus* corpus)
    : _corpus(corpus)
  { }

  TestCaseDeserializer(const TestCaseDeserializer &) = delete;
  TestCaseDeserializer(TestCaseDeserializer &&) noexcept = default;

  TestCaseDeserializer& operator=(const TestCaseDeserializer &) = delete;
  TestCaseDeserializer& operator=(TestCaseDeserializer &&) = default;

  /**
   * @brief Read a test case from the given input stream.
   *
   * @tparam Input type of the input stream. This type should contain a `read` method than can be
   * called by the following signature:
   *
   * @code
   * void read(void* buffer, size_t size);
   * @endcode
   *
   * @param in the input stream.
   * @return CAFCorpusTestCaseRef pointer to the created test case.
   */
  template <typename Input>
  CAFCorpusTestCaseRef Read(Input& in) const {
    details::TestCaseDeserializationContext context;

    auto tc = _corpus->CreateTestCase();
    auto callsCount = ReadInt<int, 4>(in);
    for (auto ci = 0; ci < callsCount; ++ci) {
      auto fc = ReadFunctionCall(in, context);
      tc->AddFunctionCall(std::move(fc));
    }

    return tc;
  }

private:
  CAFCorpus* _corpus;

  /**
   * @brief Read an integer from the given input stream.
   *
   * @tparam T the type of the integer to be returned.
   * @tparam Size the number of bytes that will be read from the stream to build the integer.
   * @tparam Input the type of the input stream.
   *
   * @param in the input stream.
   * @return T the integer read.
   */
  template <typename T, size_t Size, typename Input>
  T ReadInt(Input& in) const {
    static_assert(std::is_integral<T>::value, "T is not a trivial type.");

    uint8_t buffer[Size];
    in.read(reinterpret_cast<void *>(static_cast<uint8_t *>(buffer)), Size);

    using RawIntType = typename MakeIntegralType<Size, std::is_signed<T>::value>::Type;
    return static_cast<T>(*reinterpret_cast<RawIntType *>(static_cast<uint8_t *>(buffer)));
  }

  /**
   * @brief Read a FunctionCall object from the given stream. Values of arguments of the function
   * call can be retrieved from the given value vector.
   *
   * @tparam Input the type of the input stream.
   * @param in the input stream.
   * @param context the deserialization context.
   */
  template <typename Input>
  FunctionCall ReadFunctionCall(Input& in, details::TestCaseDeserializationContext& context) const {
    auto funcId = ReadInt<uint64_t, 4>(in);
    auto func = _corpus->store()->GetApi(funcId);

    FunctionCall fc { func.get() };

    for (auto argType : func->signature().args()) {
      // Skip the next placeholder index since the next placeholder index is reserved for the
      // current function argument.
      context.SkipNextPlaceholderIndex();
      auto value = ReadValue(in, argType.get(), context);
      fc.AddArg(value);
    }

    // Associate the next value index with the next placeholder index. The next placeholder index
    // is thus available for referencing the return value of the current function call.
    context.ReservePlaceholderIndex();
    // Skip the next value index. The next value index is reserved for the return value of the
    // current function call.
    context.SkipNextValueIndex();

    return fc;
  }

  /**
   * @brief Read a Value object from the given stream. Referenced values can be retrieved through
   * the given values vector.
   *
   * @tparam Input type of the input stream.
   * @param in the input stream.
   * @param type the type of the value to read.
   * @param context the deserialization context.
   */
  template <typename Input>
  Value* ReadValue(
      Input& in, const Type* type, details::TestCaseDeserializationContext& context) const {
    auto valueIndex = context.AllocValueIndex();

    auto pool = _corpus->GetOrCreateObjectPool(type->id());
    auto kind = static_cast<ValueKind>(ReadInt<int, 4>(in));

    Value* value = nullptr;
    switch (kind) {
      case ValueKind::BitsValue: {
        auto size = ReadInt<size_t, 4>(in);
        auto bitsValue = pool->CreateValue<BitsValue>(pool, caf::dyn_cast<BitsType>(type));
        in.read(bitsValue->data(), size);
        value = bitsValue;
        break;
      }
      case ValueKind::PointerValue: {
        auto ptrType = caf::dyn_cast<PointerType>(type);
        auto pointee = ReadValue(in, ptrType->pointeeType().get(), context);
        value = pool->CreateValue<PointerValue>(pool, pointee, ptrType);
        break;
      }
      case ValueKind::FunctionPointerValue: {
        auto funcId = ReadInt<uint64_t, 4>(in);
        value = pool->CreateValue<FunctionPointerValue>(
            pool, funcId, caf::dyn_cast<PointerType>(type));
        break;
      }
      case ValueKind::ArrayValue: {
        auto arrayType = caf::dyn_cast<ArrayType>(type);
        auto size = ReadInt<size_t, 4>(in);
        std::vector<Value *> elements;
        elements.reserve(size);
        for (size_t ei = 0; ei < size; ++ei) {
          elements.push_back(ReadValue(in, arrayType->elementType().get(), context));
        }
        value = pool->CreateValue<ArrayValue>(pool, arrayType, std::move(elements));
        break;
      }
      case ValueKind::StructValue: {
        auto structType = caf::dyn_cast<StructType>(type);
        auto ctorId = ReadInt<uint64_t, 4>(in);
        auto ctor = structType->GetConstructor(ctorId);
        std::vector<Value *> ctorArgs;
        ctorArgs.reserve(ctor->GetArgCount());
        for (size_t ai = 0; ai < ctor->GetArgCount(); ++ai) {
          auto argType = ctor->GetArgType(ai);
          ctorArgs.push_back(ReadValue(in, argType.get(), context));
        }
        value = pool->CreateValue<StructValue>(pool, structType, ctor, std::move(ctorArgs));
        break;
      }
      case ValueKind::AggregateValue: {
        auto aggregateType = caf::dyn_cast<AggregateType>(type);
        std::vector<Value *> fields;
        fields.reserve(aggregateType->GetFieldsCount());
        for (size_t i = 0; i < aggregateType->GetFieldsCount(); ++i) {
          fields.push_back(ReadValue(in, aggregateType->GetField(i).get(), context));
        }
        value = pool->CreateValue<AggregateValue>(pool, aggregateType, std::move(fields));
        break;
      }
      case ValueKind::PlaceholderValue: {
        auto valueIndex = ReadInt<size_t, 4>(in);
        if (context.IsPlaceholderIndex(valueIndex)) {
          valueIndex = context.GetPlaceholderIndex(valueIndex);
          value = _corpus->GetPlaceholderObjectPool()->CreateValue<PlaceholderValue>(
              type, valueIndex);
        } else {
          value = context.GetValue(valueIndex);
        }
        break;
      }
      default: {
        CAF_UNREACHABLE;
      }
    }

    return value;
  }
}; // class TestCaseDeserializer

} // namespace caf

#endif
