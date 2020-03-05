#include "CorpusInitialize.h"
#include "Fuzzer/TestCaseMutator.h"
#include "Fuzzer/TestCaseSerializer.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace {

class MemoryStream {
public:
  /**
   * @brief Construct a new MemoryStream object.
   *
   * @param mem the underlying memory buffer.
   */
  explicit MemoryStream(std::vector<uint8_t>& mem) noexcept
    : _mem(mem)
  { }

  /**
   * @brief Write the content of the given buffer into the underlying memory buffer.
   *
   * @param data pointer to the buffer containing data.
   * @param size size of the buffer pointed to by `data`.
   * @return MemoryStream& return `this`.
   */
  MemoryStream& write(const void* data, size_t size) noexcept {
    auto ptr = reinterpret_cast<const uint8_t *>(data);
    while (size--) {
      _mem.push_back(*ptr++);
    }

    return *this;
  }

private:
  std::vector<uint8_t>& _mem;
};

} // namespace <anonymous>

static std::unique_ptr<caf::CAFCorpus> Corpus;
static caf::Random<> Rng;
static std::vector<uint8_t> TestCaseBuffer;

extern "C" {

// For a detailed document about all the exported afl_* functions, please see
// https://github.com/vanhauser-thc/AFLplusplus/blob/master/docs/custom_mutators.md

void afl_custom_init(unsigned int seed) {
  Rng.seed(seed);
  Corpus = caf::InitCorpus();
  assert(Corpus && "Load corpus failed.");
}

size_t afl_custom_fuzz(
    uint8_t* buf, size_t buf_size,
    uint8_t* add_buf, size_t add_buf_size,
    uint8_t *mutated_out, size_t max_size) {
  assert(buf_size == sizeof(size_t) && "Incorrect buffer size.");
  auto index = *reinterpret_cast<size_t *>(buf);
  auto testCase = Corpus->GetTestCase(index);

  caf::TestCaseMutator mutator { Corpus.get(), Rng };

  auto mutatedTestCase = mutator.Mutate(testCase);
  *reinterpret_cast<size_t *>(mutated_out) = mutatedTestCase.index();

  return sizeof(size_t);
}

size_t afl_custom_pre_save(uint8_t *buf, size_t buf_size, uint8_t **out_buf) {
  auto index = *reinterpret_cast<size_t *>(buf);
  auto testCase = Corpus->GetTestCase(index);

  caf::TestCaseSerializer serializer { };
  TestCaseBuffer.clear();
  MemoryStream s { TestCaseBuffer };

  serializer.Write(s, *testCase);

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
