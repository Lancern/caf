#include "Infrastructure/Memory.h"
#include "Infrastructure/Casting.h"
#include "Basic/BitsType.h"
#include "Basic/AggregateType.h"
#include "Basic/FunctionType.h"
#include "Fuzzer/BitsValue.h"
#include "Fuzzer/PointerValue.h"
#include "Fuzzer/FunctionValue.h"
#include "Fuzzer/ArrayValue.h"
#include "Fuzzer/StructValue.h"
#include "Fuzzer/PlaceholderValue.h"
#include "Fuzzer/AggregateValue.h"
#include "Fuzzer/TestCaseMutator.h"

#include <climits>

namespace caf {

class TestCaseMutator::MutationContext {
public:
  explicit MutationContext(CAFCorpusTestCaseRef testCase)
    : _testCase(testCase)
  { }

  CAFCorpusTestCaseRef testCase() const { return _testCase; }

private:
  CAFCorpusTestCaseRef _testCase;
}; // class TestCaseMutator::MutationContext

CAFCorpusTestCaseRef TestCaseMutator::Mutate(CAFCorpusTestCaseRef testCase) {
  enum MutationStrategy : int {
    Sequence,
    Argument,
    _MutationStrategyMax = Argument
  };
  auto strategy = static_cast<MutationStrategy>(
      _rnd.Next<int>(0, static_cast<int>(_MutationStrategyMax)));
  MutationContext context { testCase };
  switch (strategy) {
    case Sequence:
      return MutateSequence(context);
    case Argument:
      return MutateArgument(context);
    default:
      CAF_UNREACHABLE;
  }
}

CAFCorpusTestCaseRef TestCaseMutator::Splice(MutationContext& context) {
  auto previous = context.testCase();
  auto source = _corpus->SelectTestCase(_rnd);
  auto previousLen = previous->calls().size();
  auto sourceLen = source->calls().size();

  auto spliceIndex = _rnd.Next<size_t>(0, std::min(previousLen, sourceLen));

  auto mutated = _corpus->CreateTestCase();
  for (size_t i = 0; i < spliceIndex; ++i) {
    mutated->AddFunctionCall(previous->calls()[i]);
  }
  for (size_t i = spliceIndex; i < sourceLen; ++i) {
    mutated->AddFunctionCall(source->calls()[i]);
  }

  return mutated;
}

CAFCorpusTestCaseRef TestCaseMutator::InsertCall(MutationContext& context) {
  auto previous = context.testCase();
  auto insertIndex = _rnd.Index(previous->calls());
  auto mutated = _corpus->CreateTestCase();
  for (size_t i = 0; i < insertIndex; ++i) {
    mutated->AddFunctionCall(previous->calls()[i]);
  }
  mutated->AddFunctionCall(_valueGen.GenerateCall());
  for (size_t i = insertIndex; i < previous->calls().size(); ++i) {
    mutated->AddFunctionCall(previous->calls()[i]);
  }

  return mutated;
}

CAFCorpusTestCaseRef TestCaseMutator::RemoveCall(MutationContext& context) {
  auto previous = context.testCase();
  auto mutated = _corpus->CreateTestCase();
  auto previousSequenceLen = previous->calls().size();
  if (previousSequenceLen == 1) {
    // TODO: Maybe change here to reject the current mutation request.
    return previous;
  }

  // Randomly choose an index at which the function call will be dropped.
  auto drop = _rnd.Index(previous->calls());

  for (size_t i = 0; i < drop; ++i) {
    mutated->AddFunctionCall(previous->calls()[i]);
  }
  for (size_t i = drop + 1; i < previousSequenceLen; ++i) {
    mutated->AddFunctionCall(previous->calls()[i]);
  }

  return mutated;
}

CAFCorpusTestCaseRef TestCaseMutator::MutateSequence(MutationContext& context) {
  enum SequenceMutationStrategy : int {
    Splice,
    InsertCall,
    RemoveCall,
    _SequenceMutationStrategyMax = RemoveCall
  };

  auto strategy = static_cast<SequenceMutationStrategy>(
      _rnd.Next(0, static_cast<int>(_SequenceMutationStrategyMax)));
  CAFCorpusTestCaseRef tc;
  switch (strategy) {
    case Splice:
      tc = this->Splice(context);
    case InsertCall:
      tc = this->InsertCall(context);
    case RemoveCall:
      tc = this->RemoveCall(context);
    default:
      CAF_UNREACHABLE;
  }

  FixPlaceholderValuesAfterSequenceMutation(tc);
  return tc;
}

void TestCaseMutator::FixPlaceholderValuesAfterSequenceMutation(CAFCorpusTestCaseRef tc) {
  for (size_t ci = 0; ci < tc->GetFunctionCallCount(); ++ci) {
    auto& call = tc->GetFunctionCall(ci);
    for (size_t ai = 0; ai < call.GetArgCount(); ++ai) {
      auto arg = call.GetArg(ai);
      if (caf::is_a<PlaceholderValue>(arg)) {
        call.SetArg(ai, _valueGen.GenerateValue(arg->type()));
      }
    }
  }
}

void TestCaseMutator::FlipBits(uint8_t* buffer, size_t size, size_t width) {
  auto offset = _rnd.Next<size_t>(0, size * CHAR_BIT - width);
  buffer[offset / CHAR_BIT] ^= ((1 << width) - 1) << (offset % CHAR_BIT);
}

void TestCaseMutator::FlipBytes(uint8_t* buffer, size_t size, size_t width) {
  auto offset = _rnd.Next<size_t>(0, size - width);
  switch (width) {
    case 1:
      buffer[offset] ^= 0xff;
      break;
    case 2:
      *reinterpret_cast<uint16_t *>(buffer + offset) ^= 0xffff;
      break;
    case 4:
      *reinterpret_cast<uint32_t *>(buffer + offset) ^= 0xffffffff;
      break;
    default:
      CAF_UNREACHABLE;
  }
}

void TestCaseMutator::Arith(uint8_t* buffer, size_t size, size_t width) {
  auto offset = _rnd.Next<size_t>(0, size - width);
  // TODO: Add code here to allow user modify the maximal absolute value of arithmetic delta.
  auto delta = _rnd.Next(-35, 35);
  switch (width) {
    case 1:
      *reinterpret_cast<int8_t *>(buffer + offset) += static_cast<int8_t>(delta);
      break;
    case 2:
      *reinterpret_cast<int16_t *>(buffer + offset) += static_cast<int16_t>(delta);
      break;
    case 4:
      *reinterpret_cast<int32_t *>(buffer + offset) += static_cast<int32_t>(delta);
      break;
    default:
      CAF_UNREACHABLE;
  }
}

Value* TestCaseMutator::MutateBitsValue(const BitsValue* value) {
  auto objectPool = value->pool();
  auto mutated = objectPool->CreateValue<BitsValue>(*value);

  enum BitsValueMutationStrategy : int {
    BitFlip1,
    BitFlip2,
    BitFlip4,
    ByteFlip1,
    ByteFlip2,
    ByteFlip4,
    ByteArith,
    WordArith,
    DWordArith,
    _MutationStrategyMax = DWordArith
  };

  std::vector<BitsValueMutationStrategy> valid {
    BitFlip1,
    BitFlip2,
    BitFlip4,
    ByteFlip1,
    ByteArith
  };
  if (value->size() >= 2) {
    valid.push_back(ByteFlip2);
    valid.push_back(WordArith);
  }
  if (value->size() >= 4) {
    valid.push_back(ByteFlip4);
    valid.push_back(DWordArith);
  }

  auto strategy = _rnd.Select(valid);
  switch (strategy) {
    case BitFlip1: {
      FlipBits(mutated->data(), mutated->size(), 1);
      break;
    }
    case BitFlip2: {
      FlipBits(mutated->data(), mutated->size(), 2);
      break;
    }
    case BitFlip4: {
      FlipBits(mutated->data(), mutated->size(), 4);
      break;
    }
    case ByteFlip1: {
      FlipBytes(mutated->data(), mutated->size(), 1);
      break;
    }
    case ByteFlip2: {
      FlipBytes(mutated->data(), mutated->size(), 2);
      break;
    }
    case ByteFlip4: {
      FlipBytes(mutated->data(), mutated->size(), 4);
      break;
    }
    case ByteArith: {
      Arith(mutated->data(), mutated->size(), 1);
      break;
    }
    case WordArith: {
      Arith(mutated->data(), mutated->size(), 2);
      break;
    }
    case DWordArith: {
      Arith(mutated->data(), mutated->size(), 4);
      break;
    }
    default: CAF_UNREACHABLE;
  }

  return mutated;
}

Value* TestCaseMutator::MutatePointerValue(const PointerValue* value, MutationContext& context) {
  auto objectPool = value->pool();
  auto mutatedPointee = MutateValue(value->pointee(), context);
  return objectPool->CreateValue<PointerValue>(objectPool, mutatedPointee,
      caf::dyn_cast<PointerType>(value->type()));
}

Value* TestCaseMutator::MutateFunctionValue(const FunctionValue* value) {
  auto objectPool = value->pool();
  if (_rnd.WithProbability(0.5)) {
    return objectPool->CreateValue<FunctionValue>(*value);
  }

  auto store = _corpus->store();
  auto functionType = caf::dyn_cast<FunctionType>(value->type());
  auto candidates = store->GetCallbackFunctions(functionType->signatureId());
  assert(candidates && "No callback function candidates viable.");

  auto funcId = _rnd.Select(*candidates);
  return objectPool->CreateValue<FunctionValue>(objectPool, funcId, functionType);
}

Value* TestCaseMutator::MutateArrayValue(const ArrayValue* value, MutationContext& context) {
  auto elementIndex = _rnd.Index(value->elements());
  auto mutatedElement = MutateValue(value->elements()[elementIndex], context);

  auto objectPool = value->pool();
  auto mutated = objectPool->CreateValue<ArrayValue>(*value);
  mutated->SetElement(elementIndex, mutatedElement);

  return mutated;
}

Value* TestCaseMutator::MutateStructValue(const StructValue* value, MutationContext& context) {
  enum StructValueMutationStrategy : int {
    MutateConstructor,
    MutateArgument,
    _StrategyMax = MutateArgument
  };

  auto strategy = MutateConstructor;
  if (!value->args().empty()) {
    strategy = static_cast<StructValueMutationStrategy>(
        _rnd.Next(0, static_cast<int>(_StrategyMax)));
  }

  switch (strategy) {
    case MutateConstructor: {
      return _valueGen.GenerateNewStructType(dynamic_cast<const StructType *>(value->type()));
    }
    case MutateArgument: {
      auto argIndex = _rnd.Index(value->args());
      auto mutatedArg = MutateValue(value->args()[argIndex], context);
      auto objectPool = value->pool();
      auto mutated = objectPool->CreateValue<StructValue>(*value);
      mutated->SetArg(argIndex, mutatedArg);
      return mutated;
    }
    default: CAF_UNREACHABLE;
  }
}

Value* TestCaseMutator::MutatePlaceholderValue(
    const PlaceholderValue* value, MutationContext& context) {
  auto tc = context.testCase();
  auto pool = value->pool();

  if (_rnd.WithProbability(0.5)) {
    // Noop.
    return pool->CreateValue<PlaceholderValue>(*value);
  }

  // Generate a Value of the corresponding type to the placeholder.
  return _valueGen.GenerateValue(value->type());
}

Value* TestCaseMutator::MutateAggregateValue(
    const AggregateValue* value, MutationContext& context) {
  auto tc = context.testCase();
  auto pool = value->pool();

  auto type = caf::dyn_cast<AggregateType>(value->type());
  if (type->GetFieldsCount() == 0) {
    return pool->CreateValue<AggregateValue>(*value);
  }

  // Choose a field to mutate.
  auto fieldIndex = _rnd.Index(type->fields());
  auto fieldType = type->GetField(fieldIndex).get();
  auto fieldValue = _valueGen.GenerateNewValue(fieldType);

  auto newValue = pool->CreateValue<AggregateValue>(pool, type);
  for (size_t i = 0; i < fieldIndex; ++i) {
    newValue->AddField(value->GetField(i));
  }
  newValue->AddField(fieldValue);
  for (size_t i = fieldIndex + 1; i < type->GetFieldsCount(); ++i) {
    newValue->AddField(value->GetField(i));
  }

  return newValue;
};

Value* TestCaseMutator::MutateValue(const Value* value, MutationContext& context) {
  switch (value->kind()) {
    case ValueKind::BitsValue: {
      return MutateBitsValue(caf::dyn_cast<BitsValue>(value));
    }
    case ValueKind::PointerValue: {
      return MutatePointerValue(caf::dyn_cast<PointerValue>(value), context);
    }
    case ValueKind::FunctionValue: {
      return MutateFunctionValue(caf::dyn_cast<FunctionValue>(value));
    }
    case ValueKind::ArrayValue: {
      return MutateArrayValue(caf::dyn_cast<ArrayValue>(value), context);
    }
    case ValueKind::StructValue: {
      return MutateStructValue(caf::dyn_cast<StructValue>(value), context);
    }
    case ValueKind::PlaceholderValue: {
      return MutatePlaceholderValue(caf::dyn_cast<PlaceholderValue>(value), context);
    }
    default: CAF_UNREACHABLE;
  }
}

CAFCorpusTestCaseRef TestCaseMutator::MutateArgument(MutationContext& context) {
  auto previous = context.testCase();
  auto functionCallIndex = _rnd.Index(previous->calls());
  auto mutated = _corpus->CreateTestCase();
  for (size_t i = 0; i < functionCallIndex; ++i) {
    mutated->AddFunctionCall(previous->calls()[i]);
  }

  auto targetCall = previous->calls()[functionCallIndex];
  if (targetCall.args().empty()) {
    // TODO: Handle this case when the selected function call does not have any arguments.
    return previous;
  } else {
    auto argIndex = _rnd.Index(targetCall.args());

    // With a probability of 0.1, we generate a new value for the argument.
    if (_rnd.WithProbability(0.1)) {
      auto value = _valueGen.GenerateValueOrPlaceholder(
          previous.get(), functionCallIndex, argIndex);
      targetCall.SetArg(argIndex, value);
    } else {
      // Mutate existing value.
      auto value = targetCall.args()[argIndex];
      value = MutateValue(value, context);
      targetCall.SetArg(argIndex, value);
    }
  }

  for (size_t i = functionCallIndex + 1; i < previous->calls().size(); ++i) {
    mutated->AddFunctionCall(previous->calls()[i]);
  }

  return mutated;
}

} // namespace caf
