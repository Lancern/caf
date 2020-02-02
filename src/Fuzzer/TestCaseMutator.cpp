#include "Basic/BitsType.h"
#include "Basic/FunctionType.h"
#include "Fuzzer/BitsValue.h"
#include "Fuzzer/PointerValue.h"
#include "Fuzzer/FunctionPointerValue.h"
#include "Fuzzer/ArrayValue.h"
#include "Fuzzer/StructValue.h"
#include "Fuzzer/TestCaseMutator.h"

#include <climits>

namespace caf {

CAFCorpusTestCaseRef TestCaseMutator::Mutate(CAFCorpusTestCaseRef testCase) {
  enum MutationStrategy : int {
    Sequence,
    Argument,
    _MutationStrategyMax = Argument
  };
  auto strategy = static_cast<MutationStrategy>(
      _rnd.Next<int>(0, static_cast<int>(_MutationStrategyMax)));
  switch (strategy) {
    case Sequence:
      return MutateSequence(testCase);
    case Argument:
      return MutateArgument(testCase);
    default:
      CAF_UNREACHABLE;
  }
}

CAFCorpusTestCaseRef TestCaseMutator::Splice(CAFCorpusTestCaseRef previous) {
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

CAFCorpusTestCaseRef TestCaseMutator::InsertCall(CAFCorpusTestCaseRef previous) {
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

CAFCorpusTestCaseRef TestCaseMutator::RemoveCall(CAFCorpusTestCaseRef previous) {
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

CAFCorpusTestCaseRef TestCaseMutator::MutateSequence(CAFCorpusTestCaseRef previous) {
  enum SequenceMutationStrategy : int {
    Splice,
    InsertCall,
    RemoveCall,
    _SequenceMutationStrategyMax = RemoveCall
  };

  auto strategy = static_cast<SequenceMutationStrategy>(
      _rnd.Next(0, static_cast<int>(_SequenceMutationStrategyMax)));
  switch (strategy) {
    case Splice:
      return this->Splice(previous);
    case InsertCall:
      return this->InsertCall(previous);
    case RemoveCall:
      return this->RemoveCall(previous);
    default:
      CAF_UNREACHABLE;
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

Value* TestCaseMutator::MutatePointerValue(const PointerValue* value) {
  auto objectPool = value->pool();
  auto mutatedPointee = MutateValue(value->pointee());
  return objectPool->CreateValue<PointerValue>(objectPool, mutatedPointee,
      dynamic_cast<const PointerType *>(value->type()));
}

Value* TestCaseMutator::MutateFunctionPointerValue(const FunctionPointerValue* value) {
  auto objectPool = value->pool();
  if (_rnd.WithProbability(0.5)) {
    return objectPool->CreateValue<FunctionPointerValue>(*value);
  }

  auto store = _corpus->store();
  auto functionType = caf::dyn_cast<FunctionType>(value->type());
  auto candidates = store->GetCallbackFunctions(functionType->signatureId());
  assert(candidates && "No callback function candidates viable.");

  auto pointeeFunctionId = _rnd.Select(*candidates);
  return objectPool->CreateValue<FunctionPointerValue>(objectPool, pointeeFunctionId, functionType);
}

Value* TestCaseMutator::MutateArrayValue(const ArrayValue* value) {
  auto elementIndex = _rnd.Index(value->elements());
  auto mutatedElement = MutateValue(value->elements()[elementIndex]);

  auto objectPool = value->pool();
  auto mutated = objectPool->CreateValue<ArrayValue>(*value);
  mutated->SetElement(elementIndex, mutatedElement);

  return mutated;
}

Value* TestCaseMutator::MutateStructValue(const StructValue* value) {
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
      auto mutatedArg = MutateValue(value->args()[argIndex]);
      auto objectPool = value->pool();
      auto mutated = objectPool->CreateValue<StructValue>(*value);
      mutated->SetArg(argIndex, mutatedArg);
      return mutated;
    }
    default: CAF_UNREACHABLE;
  }
}

Value* TestCaseMutator::MutateValue(const Value* value) {
  switch (value->kind()) {
    case ValueKind::BitsValue: {
      return MutateBitsValue(caf::dyn_cast<BitsValue>(value));
    }
    case ValueKind::PointerValue: {
      return MutatePointerValue(caf::dyn_cast<PointerValue>(value));
    }
    case ValueKind::FunctionPointerValue: {
      return MutateFunctionPointerValue(caf::dyn_cast<FunctionPointerValue>(value));
    }
    case ValueKind::ArrayValue: {
      return MutateArrayValue(caf::dyn_cast<ArrayValue>(value));
    }
    case ValueKind::StructValue: {
      return MutateStructValue(caf::dyn_cast<StructValue>(value));
    }
    default: CAF_UNREACHABLE;
  }
}

CAFCorpusTestCaseRef TestCaseMutator::MutateArgument(CAFCorpusTestCaseRef previous) {
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
    auto value = targetCall.args()[argIndex];
    value = MutateValue(value);
    targetCall.SetArg(argIndex, value);
  }

  for (size_t i = functionCallIndex + 1; i < previous->calls().size(); ++i) {
    mutated->AddFunctionCall(previous->calls()[i]);
  }

  return mutated;
}

} // namespace caf
