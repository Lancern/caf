#include "Infrastructure/Casting.h"
#include "Infrastructure/Memory.h"
#include "Fuzzer/ObjectPool.h"
#include "Fuzzer/PointerValue.h"
#include "Basic/Type.h"
#include "Basic/PointerType.h"

namespace caf {

PointerValue* CAFObjectPool::GetOrCreateNullPointerValue(const Type *pointerType) {
  assert(pointerType->kind() == TypeKind::Pointer && "The given type is not a pointer type.");
  if (!_nullPtr) {
    auto value = caf::make_unique<PointerValue>(this, caf::dyn_cast<PointerType>(pointerType));
    _nullPtr = value.get();
    _values.push_back(std::move(value));
  }
  return _nullPtr;
}

} // namespace caf
