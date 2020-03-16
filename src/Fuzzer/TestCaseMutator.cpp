#include "Infrastructure/Intrinsic.h"
#include "Fuzzer/TestCaseMutator.h"
#include "Fuzzer/Corpus.h"
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

TestCaseRef TestCaseMutator::Mutate(TestCaseRef testCase) {
  using Mutator = TestCaseRef (TestCaseMutator::*)(TestCaseRef);
  Mutator mutators[7];
  Mutator* head = mutators;

  // Can we mutate the test case by `AddFunctionCall`?
  if (testCase->GetFunctionCallsCount() < options().MaxCalls) {
    *head++ = &TestCaseMutator::AddFunctionCall;
  }

  // Can we mutate the test case by `RemoveFunctionCall`?
  if (testCase->GetFunctionCallsCount() > 1) {
    *head++ = &TestCaseMutator::RemoveFunctionCall;
  }

  // We can always mutate the test case by `Splice` and `MutateThis`.
  *head++ = &TestCaseMutator::Splice;
  *head++ = &TestCaseMutator::MutateThis;

  // Can we mutate the test case by `AddArgument`?
  for (const auto& call : *testCase) {
    if (call.GetArgsCount() < options().MaxArguments) {
      *head++ = &TestCaseMutator::AddArgument;
      break;
    }
  }

  // Can we mutate the test case by `RemoveArgument`?
  for (const auto& call : *testCase) {
    if (call.GetArgsCount() >= 1) {
      *head++ = &TestCaseMutator::RemoveArgument;
      break;
    }
  }

  // Can we mutate the test case by `MutateArgument`?
  for (const auto& call : *testCase) {
    if (call.GetArgsCount() >= 1) {
      *head++ = &TestCaseMutator::MutateArgument;
      break;
    }
  }

  assert(head > mutators && "No viable mutator.");
  auto mutator = _rnd.Select(mutators, head);
  return (this->*mutator)(testCase);
}

TestCaseRef TestCaseMutator::AddFunctionCall(TestCaseRef testCase) {
  auto call = _gen.GenerateFunctionCall();
  auto tc = _corpus.DuplicateTestCase(testCase);
  tc->PushFunctionCall(std::move(call));
  return tc;
}

TestCaseRef TestCaseMutator::RemoveFunctionCall(TestCaseRef testCase) {
  auto callIndex = _rnd.Next<size_t>(0, testCase->GetFunctionCallsCount());
  auto tc = _corpus.CreateTestCase();
  tc->ReserveFunctionCalls(testCase->GetFunctionCallsCount() - 1);
  for (size_t i = 0; i < callIndex; ++i) {
    tc->PushFunctionCall(testCase->GetFunctionCall(i));
  }
  for (size_t i = callIndex + 1; i < testCase->GetFunctionCallsCount(); ++i) {
    tc->PushFunctionCall(testCase->GetFunctionCall(i));
  }
  return tc;
}

TestCaseRef TestCaseMutator::Splice(TestCaseRef testCase) {
  // Randomly choose another test case from the corpus.
  auto another = _corpus.SelectTestCase(_rnd);
  auto prefixLen = _rnd.Next<size_t>(
      0, std::min(options().MaxCalls, testCase->GetFunctionCallsCount()));
  auto suffixLen = _rnd.Next<size_t>(
      0, std::min(options().MaxCalls - prefixLen, another->GetFunctionCallsCount()));

  auto tc = _corpus.CreateTestCase();
  for (size_t i = 0; i < prefixLen; ++i) {
    tc->PushFunctionCall(testCase->GetFunctionCall(i));
  }
  for (size_t i = another->GetFunctionCallsCount() - suffixLen;
       i < another->GetFunctionCallsCount(); ++i) {
    tc->PushFunctionCall(another->GetFunctionCall(i));
  }

  return tc;
}

TestCaseRef TestCaseMutator::MutateThis(TestCaseRef testCase) {
  auto tc = _corpus.DuplicateTestCase(testCase);
  // Choose a function call.
  auto& call = tc->SelectFunctionCall(_rnd);
  if (call.HasThis()) {
    auto thisValue = call.GetThis();
    call.SetThis(Mutate(thisValue));
  } else {
    call.SetThis(_gen.GenerateValue());
  }
  return tc;
}

TestCaseRef TestCaseMutator::AddArgument(TestCaseRef testCase) {
  auto tc = _corpus.DuplicateTestCase(testCase);

  // Collect all function calls that can accept an additional argument.
  std::vector<size_t> candidates;
  for (size_t i = 0; i < tc->GetFunctionCallsCount(); ++i) {
    const auto& call = tc->GetFunctionCall(i);
    if (call.GetArgsCount() < options().MaxArguments) {
      candidates.push_back(i);
    }
  }

  assert(!candidates.empty() && "No candidate function call viable to add additional argument.");
  auto callIndex = _rnd.Select(candidates);
  auto& call = testCase->GetFunctionCall(callIndex);
  call.PushArg(_gen.GenerateValue());

  return tc;
}

TestCaseRef TestCaseMutator::RemoveArgument(TestCaseRef testCase) {
  auto tc = _corpus.DuplicateTestCase(testCase);

  // Collect all function calls that can remove an argument.
  std::vector<size_t> candidates;
  for (size_t i = 0; i < tc->GetFunctionCallsCount(); ++i) {
    const auto& call = tc->GetFunctionCall(i);
    if (call.GetArgsCount() > 0) {
      candidates.push_back(i);
    }
  }

  assert(!candidates.empty() && "No candidate function call viable to remove argument.");
  auto callIndex = _rnd.Select(candidates);
  auto& call = tc->GetFunctionCall(callIndex);

  auto removeIndex = _rnd.Next<size_t>(0, call.GetArgsCount() - 1);
  call.RemoveArg(removeIndex);

  return tc;
}

TestCaseRef TestCaseMutator::MutateArgument(TestCaseRef testCase) {
  auto tc = _corpus.DuplicateTestCase(testCase);

  // Collect all function calls that can mutate an argument.
  std::vector<size_t> candidates;
  for (size_t i = 0; i < tc->GetFunctionCallsCount(); ++i) {
    const auto& call = tc->GetFunctionCall(i);
    if (call.GetArgsCount() > 0) {
      candidates.push_back(i);
    }
  }

  assert(!candidates.empty() && "No candidate function call viable to mutate argument.");
  auto callIndex = _rnd.Select(candidates);
  auto& call = tc->GetFunctionCall(callIndex);

  auto mutateIndex = _rnd.Next<size_t>(0, call.GetArgsCount() - 1);
  call.SetArg(mutateIndex, Mutate(call.GetArg(mutateIndex)));

  return tc;
}

Value* TestCaseMutator::Mutate(Value* value, int depth) {
  if (depth > options().MaxDepth || _rnd.WithProbability(GENERATE_NEW_VALUE_PROB)) {
    return _gen.GenerateValue();
  }

  switch (value->kind()) {
    case ValueKind::Undefined:
    case ValueKind::Null:
    case ValueKind::Function:
      return _gen.GenerateValue();
    default:
      CAF_NO_OP;
  }

  if (_rnd.WithProbability(MUTATE_TYPE_PROB)) {
    return _gen.GenerateValue();
  }

  auto& pool = _corpus.pool();
  switch (value->kind()) {
    case ValueKind::Undefined:
    case ValueKind::Null:
    case ValueKind::Function:
      CAF_UNREACHABLE;
    case ValueKind::Boolean:
      return pool.GetBooleanValue(!value->GetBooleanValue());
    case ValueKind::String:
      return MutateString(caf::dyn_cast<StringValue>(value));
    case ValueKind::Integer:
      return MutateInteger(caf::dyn_cast<IntegerValue>(value));
    case ValueKind::Float:
      return MutateFloat(caf::dyn_cast<FloatValue>(value));
    case ValueKind::Array:
      return MutateArray(caf::dyn_cast<ArrayValue>(value), depth);
    case ValueKind::Placeholder:
      return _gen.GenerateValue();
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
  return _corpus.pool().GetOrCreateStringValue(std::move(s));
}

StringValue* TestCaseMutator::RemoveCharacter(StringValue* value) {
  auto s = value->value();
  auto pos = _rnd.Index(s);
  s.erase(pos, 1);
  return _corpus.pool().GetOrCreateStringValue(std::move(s));
}

StringValue* TestCaseMutator::ChangeCharacter(StringValue* value) {
  auto s = value->value();
  auto pos = _rnd.Index(s);
  auto ch = _gen.GenerateStringCharacter();
  s[pos] = ch;
  return _corpus.pool().GetOrCreateStringValue(std::move(s));
}

StringValue* TestCaseMutator::ExchangeCharacters(StringValue* value) {
  auto s = value->value();
  auto pos1 = _rnd.Next<size_t>(0, s.length() - 2);
  auto pos2 = _rnd.Next<size_t>(pos1 + 1, s.length() - 1);
  std::swap(s[pos1], s[pos2]);
  return _corpus.pool().GetOrCreateStringValue(std::move(s));
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
  return _corpus.pool().GetOrCreateIntegerValue(newValue);
}

IntegerValue* TestCaseMutator::Negate(IntegerValue* value) {
  auto newValue = -value->value();
  return _corpus.pool().GetOrCreateIntegerValue(newValue);
}

IntegerValue* TestCaseMutator::Bitflip(IntegerValue* value) {
  constexpr static const size_t MaskLengths[] = { 1, 2, 4, 8, 16, 32 };
  auto maskLen = _rnd.Select(MaskLengths);
  auto startOffset = _rnd.Next<size_t>(0, IntegerValue::BitLength - maskLen);
  auto newValue = value->value() ^ (((1 << maskLen) - 1) << startOffset);
  return _corpus.pool().GetOrCreateIntegerValue(newValue);
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
  return _corpus.pool().GetOrCreateFloatValue(newValue);
}

FloatValue* TestCaseMutator::Negate(FloatValue* value) {
  auto newValue = -value->value();
  return _corpus.pool().GetOrCreateFloatValue(newValue);
}

ArrayValue* TestCaseMutator::MutateArray(ArrayValue* value, int depth) {
  using ArrayMutator = ArrayValue* (TestCaseMutator::*)(ArrayValue *, int);
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
  return (this->*mutator)(value, depth);
}

ArrayValue* TestCaseMutator::PushElement(ArrayValue* value, int) {
  auto element = _gen.GenerateValue();
  auto newValue = _corpus.pool().CreateArrayValue();
  newValue->reserve(value->size() + 1);
  for (size_t i = 0; i < value->size(); ++i) {
    newValue->Push(value->GetElement(i));
  }
  newValue->Push(element);
  return newValue;
}

ArrayValue* TestCaseMutator::RemoveElement(ArrayValue* value, int) {
  auto pos = _rnd.Next<size_t>(0, value->size() - 1);
  auto newValue = _corpus.pool().CreateArrayValue();
  newValue->reserve(value->size() - 1);
  for (size_t i = 0; i < pos; ++i) {
    newValue->Push(value->GetElement(i));
  }
  for (size_t i = pos + 1; i < value->size(); ++i) {
    newValue->Push(value->GetElement(i));
  }
  return newValue;
}

ArrayValue* TestCaseMutator::MutateElement(ArrayValue* value, int depth) {
  auto pos = _rnd.Next<size_t>(0, value->size());
  auto mutatedElement = Mutate(value->GetElement(pos), depth + 1);
  auto newValue = _corpus.pool().CreateArrayValue();
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

ArrayValue* TestCaseMutator::ExchangeElements(ArrayValue* value, int) {
  auto pos1 = _rnd.Next<size_t>(0, value->size() - 2);
  auto pos2 = _rnd.Next<size_t>(pos1 + 1, value->size() - 1);
  auto newValue = _corpus.pool().CreateArrayValue();
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
