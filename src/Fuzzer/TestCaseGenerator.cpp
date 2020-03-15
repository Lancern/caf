#include "Infrastructure/Memory.h"
#include "Infrastructure/Intrinsic.h"
#include "Basic/CAFStore.h"
#include "Fuzzer/TestCaseGenerator.h"
#include "Fuzzer/Corpus.h"
#include "Fuzzer/FunctionCall.h"
#include "Fuzzer/Value.h"

#include <limits>
#include <string>

constexpr static const double GENERATE_THIS_PROB = 0.5;
constexpr static const double GENERATE_DICT_INT_PROB = 0.6;
constexpr static const double CHOOSE_EXISTING_PROB = 0.2;
constexpr static const double GENERATE_DICT_FLOAT_PROB = 0.2;

constexpr static const int32_t IntegerDictionary[] = {
  -1, 0, 1, 2, 3, 4, 7, 8, 9, 15, 16, 17, 31, 32, 33, 63, 64, 65, 127, 128, 129, 255, 256, 257,
  511, 512, 513, 1023, 1024, 1025, 4095, 4096, 4097, 32767, 32768, 32769, 65535, 65536, 65537,
  std::numeric_limits<int8_t>::min(),
  std::numeric_limits<int16_t>::min(),
  std::numeric_limits<int32_t>::min(),
  std::numeric_limits<int32_t>::max(),
};

constexpr static const double FloatDictionary[] = {
  0.0, -0.0, 1.0, -1.0,
  std::numeric_limits<double>::epsilon(),
  std::numeric_limits<double>::infinity(),
  -std::numeric_limits<double>::infinity(),
  std::numeric_limits<double>::quiet_NaN(),
};

static const std::string CharacterSet =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789"
    "~!@#$%^&*()-=_+"
    "`[]\\{}|;':\",./<>?"
    "\n\t\r";

namespace caf {

TestCaseRef TestCaseGenerator::GenerateTestCase() {
  auto tc = _corpus.CreateTestCase();

  // Decide how many function calls should be included.
  auto callsCount = _rnd.Next<size_t>(1, _opt.MaxCalls);
  tc->ReserveFunctionCalls(callsCount);
  for (size_t i = 0; i < callsCount; ++i) {
    tc->PushFunctionCall(GenerateFunctionCall());
  }

  return tc;
}

FunctionCall TestCaseGenerator::GenerateFunctionCall() {
  auto calleeId = _corpus.store()->SelectFunction(_rnd).id();
  FunctionCall call { calleeId };

  // Decide whether to generate the `this` object.
  if (_rnd.WithProbability(GENERATE_THIS_PROB)) {
    // Generate `this` object.
    call.SetThis(GenerateValue());
  }

  // Decide how many arguments should be generated.
  auto argsCount = GenerateArgumentsCount();
  call.ReserveArgs(argsCount);
  for (size_t i = 0; i < argsCount; ++i) {
    call.PushArg(GenerateValue());
  }

  return call;
}

char TestCaseGenerator::GenerateStringCharacter() {
  return _rnd.Select(CharacterSet);
}

size_t TestCaseGenerator::GenerateArgumentsCount() {
  return _rnd.Next(0, 5);
}

ValueKind TestCaseGenerator::GenerateValueKind(bool generateArrayKind) {
  auto maxCode = generateArrayKind
    ? static_cast<int>(ValueKind::Array)
    : static_cast<int>(ValueKind::Float);
  return static_cast<ValueKind>(_rnd.Next(0, maxCode));
}

Value* TestCaseGenerator::GenerateValue(size_t depth) {
  // Decide whether to select an already-existing object.
  auto& pool = _corpus.pool();
  if (!pool.empty() && _rnd.WithProbability(CHOOSE_EXISTING_PROB)) {
    return pool.SelectValue(_rnd);
  }

  // Decide what kind of value to generate.
  auto kind = GenerateValueKind(depth < _opt.MaxDepth);
  switch (kind) {
    case ValueKind::Undefined:
      return pool.GetUndefinedValue();
    case ValueKind::Null:
      return pool.GetNullValue();
    case ValueKind::Function:
      return pool.GetFunctionValue();
    case ValueKind::Boolean: {
      auto value = static_cast<bool>(_rnd.Next<int>(0, 1));
      return pool.GetBooleanValue(value);
    }
    case ValueKind::String: {
      auto s = _rnd.NextString(0, _opt.MaxStringLength, CharacterSet);
      return pool.GetOrCreateStringValue(s);
    }
    case ValueKind::Integer: {
      if (_rnd.WithProbability(GENERATE_DICT_INT_PROB)) {
        return pool.GetOrCreateIntegerValue(_rnd.Select(IntegerDictionary));
      } else {
        return pool.GetOrCreateIntegerValue(_rnd.Next(
          std::numeric_limits<int32_t>::min(),
          std::numeric_limits<int32_t>::max()
        ));
      }
    }
    case ValueKind::Float: {
      if (_rnd.WithProbability(GENERATE_DICT_FLOAT_PROB)) {
        return pool.GetOrCreateFloatValue(_rnd.Select(FloatDictionary));
      } else {
        return pool.GetOrCreateFloatValue(_rnd.Next<double>());
      }
    }
    case ValueKind::Array: {
      // Decide how many elements should be included in this array.
      auto size = _rnd.Next<size_t>(0, _opt.MaxArrayLength);
      auto value = pool.CreateArrayValue();
      value->reserve(size);
      for (size_t i = 0; i < size; ++i) {
        value->Push(GenerateValue(depth + 1));
      }
      return value;
    }
    default: CAF_UNREACHABLE;
  }
}

} // namespace caf
