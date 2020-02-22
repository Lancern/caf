#include "CorpusInitialize.h"
#include "Fuzzer/TestCaseMutator.h"
#include "Fuzzer/TestCaseSerializer.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace {

std::unique_ptr<caf::CAFCorpus> corpus;

std::vector<uint8_t> testcaseBuffer;

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

extern "C" {

size_t afl_custom_mutator(uint8_t *data, size_t size, uint8_t *mutated_out,
                          size_t max_size, unsigned int seed) {
  if (!corpus) {
    corpus = caf::InitCorpus();
  }

  auto index = *reinterpret_cast<size_t *>(data);
  auto testCase = corpus->GetTestCase(index);

  caf::Random<> rnd { };
  caf::TestCaseMutator mutator { corpus.get(), rnd };

  auto mutatedTestCase = mutator.Mutate(testCase);
  *reinterpret_cast<size_t *>(mutated_out) = mutatedTestCase.index();

  return sizeof(size_t);
}

size_t afl_pre_save_handler(uint8_t *data, size_t size, uint8_t **new_data) {
  assert(corpus && "Corpus has not been loaded.");

  auto index = *reinterpret_cast<size_t *>(data);
  auto testCase = corpus->GetTestCase(index);

  caf::TestCaseSerializer serializer { };
  testcaseBuffer.clear();
  MemoryStream s { testcaseBuffer };

  serializer.Write(s, *testCase);

  *new_data = testcaseBuffer.data();
  return static_cast<size_t>(testcaseBuffer.size());
}

}
