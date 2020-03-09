#include "Basic/ArrayType.h"
#include "Basic/BitsType.h"
#include "Basic/FunctionType.h"
#include "Basic/PointerType.h"
#include "Basic/StructType.h"
#include "Basic/AggregateType.h"
#include "Basic/Type.h"
#include "Fuzzer/ArrayValue.h"
#include "Fuzzer/BitsValue.h"
#include "Fuzzer/Corpus.h"
#include "Fuzzer/FunctionValue.h"
#include "Fuzzer/PlaceholderValue.h"
#include "Fuzzer/PointerValue.h"
#include "Fuzzer/StructValue.h"
#include "Fuzzer/AggregateValue.h"
#include "Fuzzer/TestCaseGenerator.h"
#include "Fuzzer/Value.h"
#include "Infrastructure/Casting.h"
#include "Infrastructure/Intrinsic.h"

namespace caf {

BitsValue* TestCaseGenerator::GenerateNewBitsType(const BitsType* type) {
  auto objectPool = _corpus->GetOrCreateObjectPool(type->id());
  if (!objectPool->empty()) {
    auto selected = _rnd.Select(objectPool->values()).get();
    return caf::dyn_cast<BitsValue>(selected);
  }

  auto value = objectPool->CreateValue<BitsValue>(objectPool, type);
  _rnd.NextBuffer(value->data(), type->size());
  return value;
}

PointerValue* TestCaseGenerator::GenerateNewPointerType(const PointerType* type) {
  auto objectPool = _corpus->GetOrCreateObjectPool(type->id());
  if (_rnd.WithProbability(0.2)) {
    // Generate a null pointer.
    return objectPool->GetOrCreateNullPointerValue(type);
  }

  auto value = objectPool->CreateValue<PointerValue>(objectPool, type);
  value->SetPointee(GenerateValue(type->pointeeType().get()));
  return value;
}

ArrayValue* TestCaseGenerator::GenerateNewArrayType(const ArrayType* type) {
  auto objectPool = _corpus->GetOrCreateObjectPool(type->id());
  auto value = objectPool->CreateValue<ArrayValue>(objectPool, type);

  for (size_t i = 0; i < type->size(); ++i) {
    value->AddElement(GenerateValue(type->elementType().get()));
  }

  return value;
}

StructValue* TestCaseGenerator::GenerateNewStructType(const StructType* type) {
  // Choose an constructor.
  auto& constructor = _rnd.Select(type->ctors());
  const auto& constructorArgs = constructor.signature().args();

  auto objectPool = _corpus->GetOrCreateObjectPool(type->id());
  auto value =  objectPool->CreateValue<StructValue>(objectPool, type, &constructor);

  // Generate arguments passed to the constructor.
  for (size_t i = 0; i < constructorArgs.size(); ++i) {
    value->AddArg(GenerateValue(constructorArgs[i].get()));
  }

  return value;
}

FunctionValue* TestCaseGenerator::GenerateNewFunctionType(const FunctionType *type) {
  auto candidates = _corpus->store()->GetCallbackFunctions(type->signatureId());
  assert(candidates && "No callback function candidates viable.");

  auto funcId = _rnd.Select(*candidates);

  auto objectPool = _corpus->GetOrCreateObjectPool(type->id());
  return objectPool->CreateValue<FunctionValue>(objectPool, funcId, type);
}

AggregateValue* TestCaseGenerator::GenerateNewAggregateType(const AggregateType *type) {
  auto objectPool = _corpus->GetOrCreateObjectPool(type->id());
  auto value = objectPool->CreateValue<AggregateValue>(objectPool, type);

  for (size_t i = 0; i < type->GetFieldsCount(); ++i) {
    value->AddField(GenerateValue(type->GetField(i).get()));
  }

  return value;
}

Value* TestCaseGenerator::GenerateNewValue(const Type* type) {
  switch (type->kind()) {
    case TypeKind::Bits: {
      return GenerateNewBitsType(caf::dyn_cast<BitsType>(type));
    }
    case TypeKind::Pointer: {
      auto pointerType = caf::dyn_cast<PointerType>(type);
      return GenerateNewPointerType(pointerType);
    }
    case TypeKind::Array: {
      return GenerateNewArrayType(caf::dyn_cast<ArrayType>(type));
    }
    case TypeKind::Struct: {
      return GenerateNewStructType(caf::dyn_cast<StructType>(type));
    }
    case TypeKind::Function: {
      return GenerateNewFunctionType(caf::dyn_cast<FunctionType>(type));
    }
    case TypeKind::Aggregate: {
      return GenerateNewAggregateType(caf::dyn_cast<AggregateType>(type));
    }
    default: CAF_UNREACHABLE;
  }
}

Value* TestCaseGenerator::GenerateValue(const Type* type) {
  auto objectPool = _corpus->GetOrCreateObjectPool(type->id());
  if (objectPool->empty()) {
    // Does not make sense to select existing value from the object pool since it is empty.
    return GenerateNewValue(type);
  }

  if (_rnd.WithProbability(1 / std::pow(objectPool->size(), 0.3))) {
    return GenerateNewValue(type);
  } else {
    return _rnd.Select(objectPool->values()).get();
  }
}

Value* TestCaseGenerator::GenerateValueOrPlaceholder(
    const TestCase *testCase, size_t callIndex, size_t argIndex) {
  auto type = testCase->GetFunctionCall(callIndex).func()->signature().GetArgType(argIndex).get();

  // Calculate possible placeholder index candidates.
  std::vector<size_t> placeholderIndexCandidates;
  size_t valueIndex = 0;
  for (size_t i = 0; i < callIndex; ++i) {
    const auto& call = testCase->GetFunctionCall(i);
    auto retType = call.func()->signature().returnType();
    if (retType && retType.get() == type) {
      placeholderIndexCandidates.push_back(valueIndex);
    }
    ++valueIndex;
  }

  if (placeholderIndexCandidates.empty() || _rnd.WithProbability(0.5)) {
    return GenerateNewValue(type);
  }

  // Generate a placeholder.
  auto placeholderIndex = _rnd.Select(placeholderIndexCandidates);
  auto pool = _corpus->GetPlaceholderObjectPool();
  return pool->CreateValue<PlaceholderValue>(pool, type, placeholderIndex);
}

FunctionCall TestCaseGenerator::GenerateCall() {
  auto api = _rnd.Select(_corpus->store()->funcs()).get();

  FunctionCall call { api };
  for (auto arg : api->signature().args()) {
    call.AddArg(GenerateValue(arg.get()));
  }

  return call;
}

CAFCorpusTestCaseRef TestCaseGenerator::GenerateTestCase(int maxCalls) {
  assert(maxCalls >= 1 && "maxCalls cannot be less than 1.");
  auto callsCount = _rnd.Next(1, maxCalls);

  auto fc = _corpus->CreateTestCase();
  while (callsCount--) {
    fc->AddFunctionCall(GenerateCall());
  }

  return fc;
}

} // namespace caf
