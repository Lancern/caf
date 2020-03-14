#include "Infrastructure/Memory.h"
#include "Infrastructure/Random.h"
#include "Infrastructure/Stream.h"
#include "Basic/CAFStore.h"
#include "Fuzzer/Corpus.h"
#include "Fuzzer/TestCaseMutator.h"
#include "Fuzzer/TestCaseSerializer.h"
#include "Fuzzer/TestCaseDeserializer.h"

#include "json/json.hpp"

#include <ftw.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <utility>

static std::unique_ptr<caf::Corpus> Corpus;
static caf::Random<> Rng;
static std::vector<uint8_t> TestCaseBuffer;

namespace {

constexpr static const int MAX_FD_COUNTS = 4;

std::unique_ptr<caf::CAFStore> LoadCAFStore(const char* filePath) {
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

int HandleSeedFile(const char* fpath, const struct stat* sb, int typeflag) {
  if (typeflag != FTW_F) {
    return 0;
  }

  std::ifstream file { fpath };
  if (file.fail()) {
    auto code = errno;
    std::cerr << "error: failed to open " << fpath << ": "
              << std::strerror(code) << " (" << code << ")"
              << std::endl;
    std::exit(1);
  }

  caf::StlInputStream fileStream { file };
  caf::TestCaseDeserializer de { *Corpus.get(), fileStream };
  auto tc = de.Deserialize();
  if (!tc) {
    std::cerr << "error: failed to deserialize test case from seed file " << fpath << std::endl;
    std::exit(1);
  }

  return 0;
}

void PopulateCorpus(const char* seedDir) {
  if (ftw(seedDir, &HandleSeedFile, MAX_FD_COUNTS) != 0) {
    auto code = errno;
    std::cerr << "error: ftw failed: "
              << std::strerror(code) << " (" << code << ")"
              << std::endl;
    std::exit(1);
  }
}

void InitCorpus() {
  // The following environment variables should be set to initialize the corpus:
  // - CAF_STORE: Path to the cafstore.json file;
  // - CAF_SEED_DIR: Path to the directory containing seeds.
  auto storeFilePath = std::getenv("CAF_STORE");
  if (!storeFilePath) {
    std::cerr << "CAF_STORE not set." << std::endl;
    std::exit(1);
  }

  auto store = LoadCAFStore(storeFilePath);
  Corpus = caf::make_unique<caf::Corpus>(std::move(store));

  auto seedDir = std::getenv("CAF_SEED_DIR");
  if (!seedDir) {
    std::cerr << "CAF_SEED_DIR not set." << std::endl;
    std::exit(1);
  }

  PopulateCorpus(seedDir);
}

} // namespace <anonymous>

extern "C" {

// For a detailed document about all the exported afl_* functions, please see
// https://github.com/vanhauser-thc/AFLplusplus/blob/master/docs/custom_mutators.md

void afl_custom_init(unsigned int seed) {
  Rng.seed(seed);
  InitCorpus();
  assert(Corpus && "Load corpus failed.");
}

size_t afl_custom_fuzz(
    uint8_t* buf, size_t buf_size,
    uint8_t* add_buf, size_t add_buf_size,
    uint8_t *mutated_out, size_t max_size) {
  assert(buf_size == sizeof(size_t) && "Incorrect buffer size.");
  auto index = *reinterpret_cast<size_t *>(buf);
  auto testCase = Corpus->GetTestCase(index);

  caf::TestCaseMutator mutator { *Corpus.get(), Rng };

  auto mutatedTestCase = mutator.Mutate(testCase);
  *reinterpret_cast<size_t *>(mutated_out) = mutatedTestCase.index();

  return sizeof(size_t);
}

size_t afl_custom_pre_save(uint8_t *buf, size_t buf_size, uint8_t **out_buf) {
  auto index = *reinterpret_cast<size_t *>(buf);
  auto testCase = Corpus->GetTestCase(index);

  TestCaseBuffer.clear();
  caf::MemoryOutputStream s { TestCaseBuffer };
  caf::TestCaseSerializer serializer { *Corpus.get(), s };

  serializer.Serialize(*testCase);

  *out_buf = TestCaseBuffer.data();
  return static_cast<size_t>(TestCaseBuffer.size());
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
