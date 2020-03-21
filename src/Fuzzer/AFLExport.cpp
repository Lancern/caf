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

static std::unique_ptr<caf::CAFStore> Store;
static std::unique_ptr<caf::ObjectPool> Pool;
static caf::Random<> Rng;

namespace {

constexpr static const int MAX_FD_COUNTS = 4;

std::unique_ptr<caf::CAFStore> LoadCAFStore(const char* filePath) {
  std::cout << "Loading CAF metadata store from file \"" << filePath << "\"..." << std::endl;

  std::ifstream file { filePath };
  if (file.fail()) {
    auto code = errno;
    std::cerr << "error: failed to open " << filePath << ": "
              << std::strerror(code) << " (" << code << ")"
              << std::endl;
    std::exit(1);
  }

  nlohmann::json json;
  file >> json;

  return caf::make_unique<caf::CAFStore>(json);
}

} // namespace <anonymous>

extern "C" {

struct afl_state_t;

// For a detailed document about all the exported afl_* functions, please see
// https://github.com/vanhauser-thc/AFLplusplus/blob/master/docs/custom_mutators.md

void afl_custom_init(afl_state_t *, unsigned int seed) {
  auto storeFilePath = std::getenv("CAF_STORE");
  if (!storeFilePath) {
    std::cerr << "CAF_STORE not set." << std::endl;
    std::exit(1);
  }
  Store = LoadCAFStore(storeFilePath);
  if (!Store) {
    std::cerr << "Failed to load CAF store." << std::endl;
    std::exit(1);
  }

  Pool = caf::make_unique<caf::ObjectPool>();

  Rng.seed(seed);
}

size_t afl_custom_fuzz(afl_state_t *,
    uint8_t** buf, size_t buf_size,
    uint8_t* add_buf, size_t add_buf_size,
    size_t max_size) {
  Pool->clear();

  caf::MemoryInputStream primaryBufStream { *buf, buf_size };
  caf::TestCaseDeserializer de { *Pool.get(), primaryBufStream };
  auto primaryTestCase = de.Deserialize();

  caf::TestCaseMutator mutator { *Store.get(), *Pool.get(), Rng };
  caf::TestCase addTestCase;

  if (add_buf) {
    caf::MemoryInputStream addBufStream { add_buf, add_buf_size };
    caf::TestCaseDeserializer addDe { *Pool.get(), addBufStream };
    addTestCase = addDe.Deserialize();
    mutator.SetSpliceCandidate(&addTestCase);
  }

  mutator.Mutate(primaryTestCase);

  static std::vector<uint8_t> TestCaseBuffer;
  TestCaseBuffer.clear();

  caf::MemoryOutputStream outputStream { TestCaseBuffer };
  caf::TestCaseSerializer ser { outputStream };
  ser.Serialize(primaryTestCase);

  *buf = TestCaseBuffer.data();
  return TestCaseBuffer.size();
}

uint32_t afl_custom_init_trim(uint8_t* buf, size_t buf_size) {
  // We don't need AFL to trim our test case.
  return 0;
}

void afl_custom_trim(uint8_t** out_buf, size_t* out_buf_size) {
  assert(false && "afl_custom_trim should be unreachable.");
}

uint32_t afl_custom_post_trim(uint8_t success) {
  assert(false && "afl_custom_post_trim should be unreachable.");
  return 1;
}

}
