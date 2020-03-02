#include "Basic/CAFStore.h"
#include "Fuzzer/Corpus.h"

#include <cstdio>

namespace caf {

CAFCorpus::~CAFCorpus() = default;

CAFObjectPool* CAFCorpus::GetObjectPool(uint64_t typeId) const {
  auto i = _pools.find(typeId);
  if (i == _pools.end()) {
    return nullptr;
  }

  return i->second.get();
}

CAFObjectPool* CAFCorpus::GetOrCreateObjectPool(uint64_t typeId) {
  auto pool = GetObjectPool(typeId);
  if (pool) {
    return pool;
  }

  fprintf(stderr, "Creating object pool for type #%lu\n", typeId);
  auto createdPool = caf::make_unique<CAFObjectPool>();
  return _pools.emplace(typeId, std::move(createdPool)).first->second.get();
}

CAFObjectPool* CAFCorpus::GetPlaceholderObjectPool() {
  if (!_placeholderPool) {
    _placeholderPool = caf::make_unique<CAFObjectPool>();
  }
  return _placeholderPool.get();
}

} // namespace caf
