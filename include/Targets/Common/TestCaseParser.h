#ifndef CAF_TEST_CASE_PARSER_H
#define CAF_TEST_CASE_PARSER_H

#include "Infrastructure/Casting.h"
#include "Infrastructure/Intrinsic.h"
#include "Infrastructure/Memory.h"
#include "Infrastructure/Stream.h"
#include "Targets/Common/Diagnostics.h"

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>
#include <type_traits>

namespace caf {

template <typename TargetTraits>
class Target;

/**
 * @brief Parse test cases and execute them.
 *
 * @tparam TargetTraits target trait type describing the target's type system.
 */
template <typename TargetTraits>
class TestCaseParser {
public:
  using ValueType = typename TargetTraits::ValueType;

  /**
   * @brief Construct a new TestCaseParser object.
   *
   * @param target the target.
   * @param in the input stream from which the test cases will be read.
   */
  explicit TestCaseParser(Target<TargetTraits>& target, InputStream& in)
    : _target(target), _in(in)
  { }

  TestCaseParser(const TestCaseParser &) = delete;
  TestCaseParser(TestCaseParser &&) noexcept = default;

  /**
   * @brief Parse and run a test case from the underlying stream.
   *
   */
  void ParseAndRun() {
    _pool.clear();
    auto callsCount = ReadInt<size_t, 4>();
    while (callsCount--) {
      ParseCall();
    }
  }

private:
  Target<TargetTraits>& _target;
  InputStream& _in;

  std::vector<ValueType> _pool;

  template <typename Integer, size_t IntSize>
  Integer ReadInt() {
    static_assert(std::is_integral<Integer>::value, "Integer is not an integral type.");
    constexpr const bool IsSigned = std::is_signed<Integer>::value;
    using RawType = typename caf::MakeIntegralType<IntSize, IsSigned>::Type;

    uint8_t buffer[IntSize];
    _in.Read(buffer, IntSize);
    auto raw = *reinterpret_cast<RawType *>(buffer);

    return static_cast<Integer>(raw);
  }

  template <typename Literal>
  Literal ReadLiteral() {
    static_assert(std::is_literal_type<Literal>::value, "Literal is not a literal type.");

    uint8_t buffer[sizeof(Literal)];
    _in.Read(buffer, sizeof(Literal));

    return *reinterpret_cast<Literal *>(buffer);
  }

  /**
   * @brief Parse and execute a function call.
   *
   */
  void ParseCall() {
    auto funcId = ReadInt<uint32_t, 4>();
    auto thisValue = ParseValue();
    auto isCtorCall = ReadInt<uint8_t, 1>();

    auto argsCount = ReadInt<size_t, 4>();
    std::vector<ValueType> args;
    args.reserve(argsCount);

    for (size_t i = 0; i < argsCount; ++i) {
      args.push_back(ParseValue());
    }

    auto function = _target.functions().GetFunction(funcId);
    if (!function) {
      PRINT_ERR_AND_EXIT_FMT("parser: Cannot find function #%u\n", funcId);
    }

    auto ret = _target.executor().Invoke(function.take(), thisValue, isCtorCall, args);
    _pool.push_back(ret);
  }

  /**
   * @brief Parse a target-specific value.
   *
   * @return ValueType a target-specific value.
   */
  ValueType ParseValue() {
    enum ValueKind : uint8_t {
      VK_UNDEFINED,
      VK_NULL,
      VK_BOOLEAN,
      VK_STRING,
      VK_FUNCTION,
      VK_INTEGER,
      VK_FLOAT,
      VK_ARRAY,
      VK_PLACEHOLDER,
    };

    auto kind = static_cast<ValueKind>(ReadInt<uint8_t, 1>());
    switch (kind) {
      case VK_UNDEFINED:
        return _target.factory().CreateUndefined();
      case VK_NULL:
        return _target.factory().CreateNull();
      case VK_BOOLEAN:
        return ParseBooleanValue();
      case VK_STRING:
        return ParseStringValue();
      case VK_FUNCTION:
        return ParseFunctionValue();
      case VK_INTEGER:
        return ParseIntegerValue();
      case VK_FLOAT:
        return ParseFloatValue();
      case VK_ARRAY:
        return ParseArrayValue();
      case VK_PLACEHOLDER:
        return ParsePlaceholderValue();
      default:
        CAF_UNREACHABLE;
    }
  }

  typename TargetTraits::BooleanType ParseBooleanValue() {
    auto value = ReadInt<uint8_t, 1>();
    assert((value == 0 || value == 1) && "Boolean value is out of range.");
    return _target.factory().CreateBoolean(value);
  }

  typename TargetTraits::StringType ParseStringValue() {
    auto size = ReadInt<size_t, 4>();
    auto buffer = caf::make_unique<uint8_t[]>(size);
    _in.Read(buffer.get(), size);

    return _target.factory().CreateString(buffer.get(), size);
  }

  typename TargetTraits::FunctionType ParseFunctionValue() {
    auto funcId = ReadInt<uint32_t, 4>();
    return _target.factory().CreateFunction(funcId);
  }

  typename TargetTraits::IntegerType ParseIntegerValue() {
    auto value = ReadInt<int32_t, 4>();
    return _target.factory().CreateInteger(value);
  }

  typename TargetTraits::FloatType ParseFloatValue() {
    auto value = ReadLiteral<double>();
    return _target.factory().CreateFloat(value);
  }

  typename TargetTraits::ArrayType ParseArrayValue() {
    auto size = ReadInt<size_t, 4>();
    auto arrayBuilder = _target.factory().StartBuildArray(size);
    _pool.push_back(arrayBuilder.GetValue());
    for (size_t i = 0; i < size; ++i) {
      auto element = ParseValue();
      arrayBuilder.PushElement(element);
    }
    return arrayBuilder.GetValue();
  }

  typename TargetTraits::ValueType ParsePlaceholderValue() {
    auto index = ReadInt<size_t, 4>();
    return _pool.at(index);
  }
}; // class TestCaseParser

} // namespace caf

#endif
