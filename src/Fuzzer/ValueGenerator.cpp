#include "Infrastructure/Intrinsic.h"
#include "Infrastructure/Casting.h"
#include "Basic/Type.h"
#include "Basic/BitsType.h"
#include "Basic/PointerType.h"
#include "Basic/FunctionType.h"
#include "Basic/ArrayType.h"
#include "Basic/StructType.h"
#include "Fuzzer/Value.h"
#include "Fuzzer/BitsValue.h"
#include "Fuzzer/PointerValue.h"
#include "Fuzzer/FunctionPointerValue.h"
#include "Fuzzer/ArrayValue.h"
#include "Fuzzer/StructValue.h"
#include "Fuzzer/PlaceholderValue.h"
#include "Fuzzer/Corpus.h"
#include "Fuzzer/ValueGenerator.h"

namespace caf {

namespace {

size_t GetValuesCount(const Function* func) {
  size_t ret = func->signature().GetArgCount();
  if (func->signature().returnType()) {
    ++ret;
  }
  return ret;
}

}; // namespace <anonymous>

BitsValue* ValueGenerator::GenerateNewBitsType(const BitsType* type) {
  auto objectPool = _corpus->GetOrCreateObjectPool(type->id());
  if (!objectPool->empty()) {
    auto selected = _rnd.Select(objectPool->values()).get();
    return caf::dyn_cast<BitsValue>(selected);
  }

  auto value = objectPool->CreateValue<BitsValue>(objectPool, type);
  _rnd.NextBuffer(value->data(), type->size());
  return value;
}

PointerValue* ValueGenerator::GenerateNewPointerType(const PointerType* type) {
  auto objectPool = _corpus->GetOrCreateObjectPool(type->id());
  auto pointeeValue = GenerateValue(type->pointeeType().get());
  return objectPool->CreateValue<PointerValue>(objectPool, pointeeValue, type);
}

ArrayValue* ValueGenerator::GenerateNewArrayType(const ArrayType* type) {
  std::vector<Value *> elements { };
  elements.reserve(type->size());
  for (size_t i = 0; i < type->size(); ++i) {
    elements.push_back(GenerateValue(type->elementType().get()));
  }

  auto objectPool = _corpus->GetOrCreateObjectPool(type->id());
  return objectPool->CreateValue<ArrayValue>(objectPool, type, std::move(elements));
}

StructValue* ValueGenerator::GenerateNewStructType(const StructType* type) {
  // Choose an constructor.
  auto& constructor = _rnd.Select(type->ctors());
  const auto& constructorArgs = constructor.signature().args();

  // Generate arguments passed to the constructor. Note that the first argument to the constructor
  // is a pointer to the constructing object and thus should not be generated here.
  std::vector<Value *> args { };
  args.reserve(constructorArgs.size() - 1);
  for (size_t i = 1; i < constructorArgs.size(); ++i) {
    args.push_back(GenerateValue(constructorArgs[i].get()));
  }

  auto objectPool = _corpus->GetOrCreateObjectPool(type->id());
  return objectPool->CreateValue<StructValue>(objectPool, type, &constructor, std::move(args));
}

FunctionPointerValue* ValueGenerator::GenerateNewFunctionPointerType(const PointerType *type) {
  assert(type->isFunctionPointer() && "The given pointer type is not a function pointer type.");

  auto functionType = type->pointeeType().unchecked_dyn_cast<FunctionType>();
  auto candidates = _corpus->store()->GetCallbackFunctions(functionType->signatureId());
  assert(candidates && "No callback function candidates viable.");

  auto pointeeFunctionId = _rnd.Select(*candidates);

  auto objectPool = _corpus->GetOrCreateObjectPool(type->id());
  return objectPool->CreateValue<FunctionPointerValue>(objectPool, pointeeFunctionId, type);
}

Value* ValueGenerator::GenerateNewValue(const Type* type) {
  switch (type->kind()) {
    case TypeKind::Bits: {
      return GenerateNewBitsType(caf::dyn_cast<BitsType>(type));
    }
    case TypeKind::Pointer: {
      return GenerateNewPointerType(caf::dyn_cast<PointerType>(type));
    }
    case TypeKind::Array: {
      return GenerateNewArrayType(caf::dyn_cast<ArrayType>(type));
    }
    case TypeKind::Struct: {
      return GenerateNewStructType(caf::dyn_cast<StructType>(type));
    }
    default: CAF_UNREACHABLE;
  }
}

Value* ValueGenerator::GenerateValue(const Type* type) {
  auto objectPool = _corpus->GetOrCreateObjectPool(type->id());
  if (objectPool->empty()) {
    // Does not make sense to select existing value from the object pool since it is empty.
    return GenerateNewValue(type);
  }

  enum GenerateValueStrategies : int {
    UseExisting,
    CreateNew,
    _GenerateValueStrategiesMax = CreateNew
  };

  auto strategy = static_cast<GenerateValueStrategies>(
      _rnd.Next(0, static_cast<int>(_GenerateValueStrategiesMax)));
  switch (strategy) {
    case UseExisting: {
      // Randomly choose an existing value from the object pool.
      return _rnd.Select(objectPool->values()).get();
    }
    case CreateNew: {
      return GenerateNewValue(type);
    }
    default: CAF_UNREACHABLE;
  }
}

Value* ValueGenerator::GenerateValueOrPlaceholder(
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
    valueIndex += GetValuesCount(call.func());
  }

  if (placeholderIndexCandidates.empty() || _rnd.WithProbability(0.5)) {
    return GenerateNewValue(type);
  }

  // Generate a placeholder.
  auto placeholderIndex = _rnd.Select(placeholderIndexCandidates);
  auto pool = _corpus->GetPlaceholderObjectPool();
  return pool->CreateValue<PlaceholderValue>(pool, type, placeholderIndex);
}

FunctionCall ValueGenerator::GenerateCall() {
  auto api = _rnd.Select(_corpus->store()->funcs()).get();

  FunctionCall call { api };
  for (auto arg : api->signature().args()) {
    call.AddArg(GenerateValue(arg.get()));
  }

  return call;
}

} // namespace caf
