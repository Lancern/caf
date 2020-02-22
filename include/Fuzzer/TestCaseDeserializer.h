#ifndef CAF_TEST_CASE_DESERIALIZER_H
#define CAF_TEST_CASE_DESERIALIZER_H

#include "Infrastructure/Casting.h"
#include "Infrastructure/Intrinsic.h"
#include "Basic/CAFStore.h"
#include "Basic/Function.h"
#include "Basic/Type.h"
#include "Basic/BitsType.h"
#include "Basic/PointerType.h"
#include "Basic/ArrayType.h"
#include "Basic/StructType.h"
#include "Basic/Constructor.h"
#include "Corpus.h"
#include "ObjectPool.h"
#include "FunctionCall.h"
#include "Value.h"
#include "BitsValue.h"
#include "PointerValue.h"
#include "FunctionPointerValue.h"
#include "ArrayValue.h"
#include "StructValue.h"

#include <cstdint>
#include <utility>
#include <type_traits>
#include <vector>

namespace caf {

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

  template <typename Input>
  CAFCorpusTestCaseRef Read(Input& in) const {
    std::vector<Value *> values;

    auto tc = _corpus->CreateTestCase();
    auto callsCount = ReadInt<int, 4>(in);
    for (auto ci = 0; ci < callsCount; ++ci) {
      auto fc = ReadFunctionCall(in, values);
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
   * @tparam Input the type of the input stream. This type should contain a `read` method than can
   * be called by the following signature:
   *
   * @code
   * void read(uint8_t* buffer, size_t size);
   * @endcode
   *
   * @param in the input stream.
   * @return T the integer read.
   */
  template <typename T, size_t Size, typename Input>
  T ReadInt(Input& in) const {
    static_assert(std::is_integral<T>::value, "T is not a trivial type.");

    uint8_t buffer[Size];
    in.read(buffer, Size);

    using RawIntType = typename MakeIntegralType<Size, std::is_signed<T>::value>::Type;
    return static_cast<T>(*reinterpret_cast<RawIntType *>(static_cast<uint8_t *>(buffer)));
  }

  /**
   * @brief Read a FunctionCall object from the given stream. Values of arguments of the function
   * call can be retrieved from the given value vector.
   *
   * @tparam Input the type of the input stream.
   * @param in the input stream.
   * @param values a Value vector from which the arguments of the function call can be retrieved.
   * @return FunctionCall the object read.
   */
  template <typename Input>
  FunctionCall ReadFunctionCall(Input& in, std::vector<Value *>& values) const {
    auto funcId = ReadInt<uint64_t, 4>(in);
    auto func = _corpus->store()->GetApi(funcId);

    FunctionCall fc { func.get() };

    // Add a nullptr to values. This nullptr acts as a placeholder for the return value of the
    // current function.
    values.push_back(nullptr);

    for (auto argType : func->args()) {
      auto value = ReadValue(in, argType.get(), values);
      fc.AddArg(value);
      values.push_back(value);
    }

    return fc;
  }

  /**
   * @brief Read a Value object from the given stream. Referenced values can be retrieved through
   * the given values vector.
   *
   * @tparam Input type of the input stream.
   * @param in the input stream.
   * @param type the type of the value to read.
   * @param values a vector of values that is used to retrieve referenced values.
   * @return Value* the value read.
   */
  template <typename Input>
  Value* ReadValue(Input& in, const Type* type, const std::vector<Value *>& values) const {
    auto pool = _corpus->GetOrCreateObjectPool(type->id());
    auto kind = static_cast<ValueKind>(ReadInt<int, 4>(in));
    switch (kind) {
      case ValueKind::BitsValue: {
        auto size = ReadInt<size_t, 4>(in);
        auto value = pool->CreateValue<BitsValue>(pool, caf::dyn_cast<BitsType>(type));
        in.read(value->data(), size);
        return value;
      }
      case ValueKind::PointerValue: {
        auto ptrType = caf::dyn_cast<PointerType>(type);
        auto pointee = ReadValue(in, ptrType->pointeeType(), values);
        return pool->CreateValue<PointerValue>(pool, pointee, ptrType);
      }
      case ValueKind::FunctionPointerValue: {
        auto funcId = ReadInt<uint64_t, 4>(in);
        return pool->CreateValue<FunctionPointerValue>(
            pool, funcId, caf::dyn_cast<PointerType>(type));
      }
      case ValueKind::ArrayValue: {
        auto arrayType = caf::dyn_cast<ArrayType>(type);
        auto size = ReadInt<size_t, 4>(in);
        std::vector<Value *> elements;
        elements.reserve(size);
        for (size_t ei = 0; ei < size; ++ei) {
          elements.push_back(ReadValue(in, arrayType->elementType(), values));
        }
        return pool->CreateValue<ArrayValue>(pool, arrayType, std::move(elements));
      }
      case ValueKind::StructValue: {
        auto structType = caf::dyn_cast<StructType>(type);
        auto ctorId = ReadInt<uint64_t, 4>(in);
        auto ctor = structType->GetConstructor(ctorId);
        std::vector<Value *> ctorArgs;
        ctorArgs.reserve(ctor->GetArgCount());
        for (size_t ai = 0; ai < ctor->GetArgCount(); ++ai) {
          auto argType = ctor->GetArgType(ai);
          ctorArgs.push_back(ReadValue(in, argType.get(), values));
        }
        return pool->CreateValue<StructValue>(pool, structType, ctor, std::move(ctorArgs));
      }
      case ValueKind::PlaceholderValue: {
        auto valueIndex = ReadInt<size_t, 4>(in);
        return values.at(valueIndex);
      }
      default: {
        CAF_UNREACHABLE;
      }
    }
  }
}; // class TestCaseDeserializer

} // namespace caf

#endif
