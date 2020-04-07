#include "Infrastructure/Intrinsic.h"
#include "Fuzzer/TestCaseMutator.h"
#include "Fuzzer/ObjectPool.h"
#include "Fuzzer/TestCase.h"
#include "Fuzzer/FunctionCall.h"
#include "Fuzzer/Value.h"

#include <utility>
#include <iterator>
#include <algorithm>

namespace caf {

constexpr static const double GENERATE_NEW_VALUE_PROB = 0.1;
constexpr static const double MUTATE_TYPE_PROB = 0.2;

constexpr static const int32_t INTEGER_MAX_INCREMENT = 10;
constexpr static const int32_t INTEGER_MIN_INCREMENT = -10;

constexpr static const double FLOAT_MAX_INCREMENT = 100;
constexpr static const double FLOAT_MIN_INCREMENT = -100;

void TestCaseMutator::Mutate(TestCase& testCase) {
  using Mutator = void (TestCaseMutator::*)(TestCase &);
  Mutator mutators[8];
  Mutator* head = mutators;

  // Can we mutate the test case by `AddFunctionCall`?
  if (testCase.GetFunctionCallsCount() < options().MaxCalls) {
    *head++ = &TestCaseMutator::AddFunctionCall;
  }

  // Can we mutate the test case by `RemoveFunctionCall`?
  if (testCase.GetFunctionCallsCount() > 1) {
    *head++ = &TestCaseMutator::RemoveFunctionCall;
  }

  // Can we mutate the test case by `Splice`?
  if (HasSpliceCandidate()) {
    *head++ = &TestCaseMutator::Splice;
  }

  // We can always mutate the test case by `MutateThis` and `MutateCtor`.
  *head++ = &TestCaseMutator::MutateThis;
  *head++ = &TestCaseMutator::MutateCtor;

  // Can we mutate the test case by `AddArgument`?
  for (const auto& call : testCase) {
    if (call.GetArgsCount() < options().MaxArguments) {
      *head++ = &TestCaseMutator::AddArgument;
      break;
    }
  }

  // Can we mutate the test case by `RemoveArgument`?
  for (const auto& call : testCase) {
    if (call.GetArgsCount() >= 1) {
      *head++ = &TestCaseMutator::RemoveArgument;
      break;
    }
  }

  // Can we mutate the test case by `MutateArgument`?
  for (const auto& call : testCase) {
    if (call.GetArgsCount() >= 1) {
      *head++ = &TestCaseMutator::MutateArgument;
      break;
    }
  }

  assert(head > mutators && "No viable mutator.");
  auto mutator = _rnd.Select(mutators, head);
  (this->*mutator)(testCase);
}

template <typename Fixer>
void TestCaseMutator::FixPlaceholderValues(TestCase& testCase, size_t startCallIndex, Fixer fixer) {
  for (size_t i = startCallIndex; i < testCase.GetFunctionCallsCount(); ++i) {
    auto& call = testCase.GetFunctionCall(i);
    if (call.HasThis() && call.GetThis()->IsPlaceholder()) {
      auto newValue = fixer(i, call.GetThis()->GetPlaceholderIndex());
      call.SetThis(newValue);
    }
    for (size_t ai = 0; ai < call.GetArgsCount(); ++ai) {
      auto arg = call.GetArg(ai);
      if (!arg->IsPlaceholder()) {
        continue;
      }
      auto newArg = fixer(i, arg->GetPlaceholderIndex());
      call.SetArg(ai, newArg);
    }
  }
}

void TestCaseMutator::AddFunctionCall(TestCase& testCase) {
  auto index = _rnd.Next<size_t>(0, testCase.GetFunctionCallsCount());
  auto call = _gen.GenerateFunctionCall(index);
  testCase.InsertFunctionCall(index, std::move(call));

  // Fix all placeholder values that reference to functions whose index is greater than or equal to
  // the inserted index.
  FixPlaceholderValues(testCase, index + 1,
      [index, this] (size_t, size_t placeholderIndex) -> Value * {
        if (placeholderIndex >= index) {
          ++placeholderIndex;
        }
        return _pool.GetPlaceholderValue(placeholderIndex);
      });
}

void TestCaseMutator::RemoveFunctionCall(TestCase& testCase) {
  auto index = _rnd.Next<size_t>(0, testCase.GetFunctionCallsCount() - 1);
  testCase.RemoveFunctionCall(index);

  // Fix all placeholder values that references to functions whose index is greater than or equal to
  // the removed index.
  FixPlaceholderValues(testCase, index,
      [index, this] (size_t callIndex, size_t placeholderIndex) -> Value * {
        if (placeholderIndex == index) {
          return _gen.GenerateValue(
              TestCaseGenerator::GeneratePlaceholderValueParams { callIndex });
        } else if (placeholderIndex > index) {
          --placeholderIndex;
        }
        return _pool.GetPlaceholderValue(placeholderIndex);
      });
}

void TestCaseMutator::Splice(TestCase& testCase) {
  assert(_spliceCandidate && "No splice candidate set.");

  auto prefixLen = _rnd.Next<size_t>(
      0, std::min(options().MaxCalls, testCase.GetFunctionCallsCount()));
  auto suffixLen = _rnd.Next<size_t>(
      0, std::min(options().MaxCalls - prefixLen, _spliceCandidate->GetFunctionCallsCount()));

  std::vector<FunctionCall> calls;
  calls.reserve(suffixLen);
  for (size_t i = _spliceCandidate->GetFunctionCallsCount() - suffixLen;
       i < _spliceCandidate->GetFunctionCallsCount(); ++i) {
    calls.push_back(_spliceCandidate->GetFunctionCall(i));
  }

  testCase.RemoveTailCalls(prefixLen);
  testCase.AppendFunctionCalls(std::move(calls));

  // TODO: Fix all placeholder values that reference to function calls whose index is greater than
  // TODO: or equal to prefixLen.
}

void TestCaseMutator::MutateThis(TestCase& testCase) {
  // Choose a function call.
  auto callIndex = _rnd.Next<size_t>(0, testCase.GetFunctionCallsCount() - 1);
  auto& call = testCase.GetFunctionCall(callIndex);
  if (call.HasThis()) {
    auto thisValue = call.GetThis();
    call.SetThis(Mutate(thisValue, callIndex));
  } else {
    call.SetThis(_gen.GenerateValue(
        TestCaseGenerator::GeneratePlaceholderValueParams { callIndex }));
  }
}

void TestCaseMutator::MutateCtor(TestCase& testCase) {
  // Choose a function call.
  auto callIndex = _rnd.Next<size_t>(0, testCase.GetFunctionCallsCount() - 1);
  auto& call = testCase.GetFunctionCall(callIndex);
  call.SetConstructorCall(!call.IsConstructorCall());
}

void TestCaseMutator::AddArgument(TestCase& testCase) {
  // Collect all function calls that can accept an additional argument.
  std::vector<size_t> candidates;
  for (size_t i = 0; i < testCase.GetFunctionCallsCount(); ++i) {
    const auto& call = testCase.GetFunctionCall(i);
    if (call.GetArgsCount() < options().MaxArguments) {
      candidates.push_back(i);
    }
  }

  assert(!candidates.empty() && "No candidate function call viable to add additional argument.");
  auto callIndex = _rnd.Select(candidates);
  auto& call = testCase.GetFunctionCall(callIndex);
  call.PushArg(_gen.GenerateValue(
    TestCaseGenerator::GeneratePlaceholderValueParams { callIndex }));
}

void TestCaseMutator::RemoveArgument(TestCase& testCase) {
  // Collect all function calls that can remove an argument.
  std::vector<size_t> candidates;
  for (size_t i = 0; i < testCase.GetFunctionCallsCount(); ++i) {
    const auto& call = testCase.GetFunctionCall(i);
    if (call.GetArgsCount() > 0) {
      candidates.push_back(i);
    }
  }

  assert(!candidates.empty() && "No candidate function call viable to remove argument.");
  auto callIndex = _rnd.Select(candidates);
  auto& call = testCase.GetFunctionCall(callIndex);

  auto removeIndex = _rnd.Next<size_t>(0, call.GetArgsCount() - 1);
  call.RemoveArg(removeIndex);
}

void TestCaseMutator::MutateArgument(TestCase& testCase) {
  // Collect all function calls that can mutate an argument.
  std::vector<size_t> candidates;
  for (size_t i = 0; i < testCase.GetFunctionCallsCount(); ++i) {
    const auto& call = testCase.GetFunctionCall(i);
    if (call.GetArgsCount() > 0) {
      candidates.push_back(i);
    }
  }

  assert(!candidates.empty() && "No candidate function call viable to mutate argument.");
  auto callIndex = _rnd.Select(candidates);
  auto& call = testCase.GetFunctionCall(callIndex);

  auto mutateIndex = _rnd.Next<size_t>(0, call.GetArgsCount() - 1);
  call.SetArg(mutateIndex, Mutate(call.GetArg(mutateIndex), callIndex));
}

Value* TestCaseMutator::Mutate(Value* value, size_t callIndex, int depth) {
  TestCaseGenerator::GeneratePlaceholderValueParams params { callIndex };

  if (depth > options().MaxDepth || _rnd.WithProbability(GENERATE_NEW_VALUE_PROB)) {
    return _gen.GenerateValue(params);
  }

  switch (value->kind()) {
    case ValueKind::Undefined:
    case ValueKind::Null:
      return _gen.GenerateValue(params);
    default:
      CAF_NO_OP;
  }

  if (_rnd.WithProbability(MUTATE_TYPE_PROB)) {
    return _gen.GenerateValue(params);
  }

  switch (value->kind()) {
    case ValueKind::Undefined:
    case ValueKind::Null:
      CAF_UNREACHABLE;
    case ValueKind::Function:
      return _gen.GenerateFunctionValue();
    case ValueKind::Boolean:
      return _pool.GetBooleanValue(!value->GetBooleanValue());
    case ValueKind::String:
      return MutateString(caf::dyn_cast<StringValue>(value));
    case ValueKind::Integer:
      return MutateInteger(caf::dyn_cast<IntegerValue>(value));
    case ValueKind::Float:
      return MutateFloat(caf::dyn_cast<FloatValue>(value));
    case ValueKind::Array:
      return MutateArray(caf::dyn_cast<ArrayValue>(value), callIndex, depth);
    case ValueKind::Placeholder:
      return _gen.GenerateValue(params);
    default:
      CAF_UNREACHABLE;
  }

  return nullptr; // Make compiler happy
}

StringValue* TestCaseMutator::MutateString(StringValue* value) {
  using StringMutator = StringValue* (TestCaseMutator::*)(StringValue *);
  StringMutator mutators[4];
  StringMutator* head = mutators;

  // Can we mutate the string by `InsertCharacter`?
  if (value->length() < options().MaxStringLength) {
    *head++ = &TestCaseMutator::InsertCharacter;
  }

  // Can we mutate the string by `RemoveCharacter`?
  if (value->length() > 0) {
    *head++ = &TestCaseMutator::RemoveCharacter;
  }

  // Can we mutate the string by `ChangeCharacter`?
  if (value->length() > 0) {
    *head++ = &TestCaseMutator::ChangeCharacter;
  }

  // Can we mutate the string by `ExchangeCharacters`?
  if (value->length() >= 2) {
    *head++ = &TestCaseMutator::ExchangeCharacters;
  }

  assert(head != mutators && "No viable string value mutators.");
  auto mutator = _rnd.Select(mutators, head);
  return (this->*mutator)(value);
}

StringValue* TestCaseMutator::InsertCharacter(StringValue* value) {
  auto s = value->value();
  auto pos = _rnd.Next<size_t>(0, s.length());
  auto ch = _gen.GenerateStringCharacter();
  s.insert(pos, 1, ch);
  return _pool.GetOrCreateStringValue(std::move(s));
}

StringValue* TestCaseMutator::RemoveCharacter(StringValue* value) {
  auto s = value->value();
  auto pos = _rnd.Index(s);
  s.erase(pos, 1);
  return _pool.GetOrCreateStringValue(std::move(s));
}

StringValue* TestCaseMutator::ChangeCharacter(StringValue* value) {
  auto s = value->value();
  auto pos = _rnd.Index(s);
  auto ch = _gen.GenerateStringCharacter();
  s[pos] = ch;
  return _pool.GetOrCreateStringValue(std::move(s));
}

StringValue* TestCaseMutator::ExchangeCharacters(StringValue* value) {
  auto s = value->value();
  auto pos1 = _rnd.Next<size_t>(0, s.length() - 2);
  auto pos2 = _rnd.Next<size_t>(pos1 + 1, s.length() - 1);
  std::swap(s[pos1], s[pos2]);
  return _pool.GetOrCreateStringValue(std::move(s));
}

IntegerValue* TestCaseMutator::MutateInteger(IntegerValue* value) {
  using IntegerMutator = IntegerValue* (TestCaseMutator::*)(IntegerValue *);
  constexpr static const IntegerMutator mutators[] = {
    &TestCaseMutator::Increment,
    &TestCaseMutator::Negate,
    &TestCaseMutator::Bitflip
  };
  auto mutator = _rnd.Select(mutators);
  return (this->*mutator)(value);
}

IntegerValue* TestCaseMutator::Increment(IntegerValue* value) {
  auto inc = _rnd.Next<int32_t>(INTEGER_MIN_INCREMENT, INTEGER_MAX_INCREMENT);
  auto newValue = static_cast<int32_t>(
      static_cast<uint32_t>(value->value()) + static_cast<uint32_t>(inc));
  return _pool.GetOrCreateIntegerValue(newValue);
}

IntegerValue* TestCaseMutator::Negate(IntegerValue* value) {
  auto newValue = -value->value();
  return _pool.GetOrCreateIntegerValue(newValue);
}

IntegerValue* TestCaseMutator::Bitflip(IntegerValue* value) {
  constexpr static const size_t MaskLengths[] = { 1, 2, 4, 8, 16, 32 };
  auto maskLen = _rnd.Select(MaskLengths);
  auto startOffset = _rnd.Next<size_t>(0, IntegerValue::BitLength - maskLen);
  auto newValue = value->value() ^ (((1 << maskLen) - 1) << startOffset);
  return _pool.GetOrCreateIntegerValue(newValue);
}

FloatValue* TestCaseMutator::MutateFloat(FloatValue* value) {
  if (std::isnan(value->value())) {
    return value;
  }

  using FloatMutator = FloatValue* (TestCaseMutator::*)(FloatValue *);
  constexpr static const FloatMutator mutators[2] = {
    &TestCaseMutator::Increment,
    &TestCaseMutator::Negate
  };
  auto mutator = _rnd.Select(mutators);
  return (this->*mutator)(value);
}

FloatValue* TestCaseMutator::Increment(FloatValue* value) {
  auto inc = _rnd.Next(FLOAT_MIN_INCREMENT, FLOAT_MAX_INCREMENT);
  auto newValue = value->value() + inc;
  return _pool.GetOrCreateFloatValue(newValue);
}

FloatValue* TestCaseMutator::Negate(FloatValue* value) {
  auto newValue = -value->value();
  return _pool.GetOrCreateFloatValue(newValue);
}

ArrayValue* TestCaseMutator::MutateArray(ArrayValue* value, size_t callIndex, int depth) {
  using ArrayMutator = ArrayValue* (TestCaseMutator::*)(ArrayValue *, size_t, int);
  ArrayMutator mutators[4];
  ArrayMutator *head = mutators;

  // Can we mutate the value using `PushElement`?
  if (value->size() < options().MaxArrayLength) {
    *head++ = &TestCaseMutator::PushElement;
  }

  // Can we mutate the value using `RemoveElement`?
  if (value->size() > 0) {
    *head++ = &TestCaseMutator::RemoveElement;
  }

  // Can we mutate the value using `MutateElement`?
  if (value->size() > 0) {
    *head++ = &TestCaseMutator::MutateElement;
  }

  // Can we mutate the value using `ExchangeElements`?
  if (value->size() >= 2) {
    *head++ = &TestCaseMutator::ExchangeElements;
  }

  assert(head != mutators && "No viable array mutators.");
  auto mutator = _rnd.Select(mutators, head);
  return (this->*mutator)(value, callIndex, depth);
}

ArrayValue* TestCaseMutator::PushElement(ArrayValue* value, size_t callIndex, int) {
  auto element = _gen.GenerateValue(
      TestCaseGenerator::GeneratePlaceholderValueParams { callIndex });
  auto newValue = _pool.CreateArrayValue();
  newValue->reserve(value->size() + 1);
  for (size_t i = 0; i < value->size(); ++i) {
    newValue->Push(value->GetElement(i));
  }
  newValue->Push(element);
  return newValue;
}

ArrayValue* TestCaseMutator::RemoveElement(ArrayValue* value, size_t, int) {
  auto pos = _rnd.Next<size_t>(0, value->size() - 1);
  auto newValue = _pool.CreateArrayValue();
  newValue->reserve(value->size() - 1);
  for (size_t i = 0; i < pos; ++i) {
    newValue->Push(value->GetElement(i));
  }
  for (size_t i = pos + 1; i < value->size(); ++i) {
    newValue->Push(value->GetElement(i));
  }
  return newValue;
}

ArrayValue* TestCaseMutator::MutateElement(ArrayValue* value, size_t callIndex, int depth) {
  auto pos = _rnd.Next<size_t>(0, value->size() - 1);
  auto mutatedElement = Mutate(value->GetElement(pos), callIndex, depth + 1);
  auto newValue = _pool.CreateArrayValue();
  newValue->reserve(value->size());
  for (size_t i = 0; i < pos; ++i) {
    newValue->Push(value->GetElement(i));
  }
  newValue->Push(mutatedElement);
  for (size_t i = pos + 1; i < value->size(); ++i) {
    newValue->Push(value->GetElement(i));
  }
  return newValue;
}

ArrayValue* TestCaseMutator::ExchangeElements(ArrayValue* value, size_t, int) {
  auto pos1 = _rnd.Next<size_t>(0, value->size() - 2);
  auto pos2 = _rnd.Next<size_t>(pos1 + 1, value->size() - 1);
  auto newValue = _pool.CreateArrayValue();
  newValue->reserve(value->size());
  for (size_t i = 0; i < value->size(); ++i) {
    if (i == pos1) {
      newValue->Push(value->GetElement(pos2));
    } else if (i == pos2) {
      newValue->Push(value->GetElement(pos1));
    } else {
      newValue->Push(value->GetElement(i));
    }
  }
  return newValue;
}

} // namespace caf
