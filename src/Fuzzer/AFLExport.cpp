#include "Infrastructure/Memory.h"
#include "Infrastructure/Random.h"
#include "Infrastructure/Stream.h"
#include "Basic/CAFStore.h"
#include "Fuzzer/ObjectPool.h"
#include "Fuzzer/TestCaseMutator.h"
#include "Fuzzer/TestCaseSerializer.h"
#include "Fuzzer/TestCaseDeserializer.h"

#include "json/json.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <utility>

namespace {

std::unique_ptr<caf::CAFStore> Store;
std::vector<uint8_t> Buffer;

void LoadCAFStore() {
  if (Store) {
    return;
  }

  auto storeFilePath = std::getenv("CAF_STORE");
  if (!storeFilePath) {
    std::cerr << "CAF_STORE not set." << std::endl;
    std::exit(1);
  }

  std::cout << "Loading CAF metadata store from file \"" << storeFilePath << "\"..." << std::endl;

  std::ifstream file { storeFilePath };
  if (file.fail()) {
    auto code = errno;
    std::cerr << "error: failed to open " << storeFilePath << ": "
              << std::strerror(code) << " (" << code << ")"
              << std::endl;
    std::exit(1);
  }

  nlohmann::json json;
  file >> json;

  Store = caf::make_unique<caf::CAFStore>(json);
}

} // namespace <anonymous>

extern "C" {

// For a detailed document about all the exported afl_* functions, please see
// https://github.com/vanhauser-thc/AFLplusplus/blob/master/docs/custom_mutators.md

size_t afl_custom_mutator(
    uint8_t *data, size_t size,
    uint8_t *mutated_out, size_t max_size,
    unsigned int seed) {
  if (!Store) {
    LoadCAFStore();
  }

  caf::ObjectPool pool { };
  caf::MemoryInputStream primaryBufStream { data, size };
  caf::TestCaseDeserializer de { pool, primaryBufStream };
  auto primaryTestCase = de.Deserialize();

  caf::Random<> rng { };
  rng.seed(seed);
  caf::TestCaseMutator mutator { *Store, pool, rng };
  caf::TestCase addTestCase;

  // if (add_buf) {
  //   caf::MemoryInputStream addBufStream { add_buf, add_buf_size };
  //   caf::TestCaseDeserializer addDe { pool, addBufStream };
  //   addTestCase = addDe.Deserialize();
  //   mutator.SetSpliceCandidate(&addTestCase);
  // }

  mutator.Mutate(primaryTestCase);

  Buffer.clear();
  caf::MemoryOutputStream outputStream { Buffer };
  caf::TestCaseSerializer ser { outputStream };
  ser.Serialize(primaryTestCase);

  auto mutatedSize = Buffer.size();
  assert(mutatedSize <= max_size && "Mutated size is too large.");
  std::memcpy(mutated_out, Buffer.data(), mutatedSize);

  return mutatedSize;
}

}
