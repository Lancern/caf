#include "Infrastructure/Intrinsic.h"
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
#include "Fuzzer/Corpus.h"
#include "Fuzzer/ValueGenerator.h"

namespace caf {

Value* ValueGenerator::GenerateNewBitsType(const BitsType* type) {
  auto objectPool = _corpus->GetObjectPool(type->id());
  if (!objectPool) {
    // TODO: Refactor here to reject current value generation request.
    return nullptr;
  }

  return _rnd.Select(objectPool->values()).get();
}

Value* ValueGenerator::GenerateNewPointerType(const PointerType* type) {
  auto objectPool = _corpus->GetOrCreateObjectPool(type->id());
  auto pointeeValue = GenerateValue(type->pointeeType().get());
  return objectPool->CreateValue<PointerValue>(objectPool, pointeeValue, type);
}

Value* ValueGenerator::GenerateNewArrayType(const ArrayType* type) {
  std::vector<Value *> elements { };
  elements.reserve(type->size());
  for (size_t i = 0; i < type->size(); ++i) {
    elements.push_back(GenerateValue(type->elementType().get()));
  }

  auto objectPool = _corpus->GetOrCreateObjectPool(type->id());
  return objectPool->CreateValue<ArrayValue>(objectPool, type, std::move(elements));
}

Value* ValueGenerator::GenerateNewStructType(const StructType* type) {
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
  return objectPool->CreateValue<StructValue>(objectPool, type, constructor, std::move(args));
}

Value* ValueGenerator::GenerateNewFunctionPointerType(const PointerType *type) {
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
      return GenerateNewBitsType(dynamic_cast<const BitsType *>(type));
    }
    case TypeKind::Pointer: {
      return GenerateNewPointerType(dynamic_cast<const PointerType *>(type));
    }
    case TypeKind::Array: {
      return GenerateNewArrayType(dynamic_cast<const ArrayType *>(type));
    }
    case TypeKind::Struct: {
      return GenerateNewStructType(dynamic_cast<const StructType *>(type));
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

FunctionCall ValueGenerator::GenerateCall() {
  auto api = _rnd.Select(_corpus->store()->funcs()).get();

  FunctionCall call { api };
  for (auto arg : api->signature().args()) {
    call.AddArg(GenerateValue(arg.get()));
  }

  return call;
}

} // namespace caf
