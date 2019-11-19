#include <cstddef>
#include <cstdint>
#include <memory>

#include "CAFMutator.hpp"


std::unique_ptr<caf::CAFCorpus> _corpus;

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
  MemoryStream& write(const uint8_t* data, size_t size) noexcept {
    while (size--) {
      _mem.push_back(*data++);
    }

    return *this;
  }

private:
  std::vector<uint8_t>& _mem;
};


extern "C" {

void caf_corpus_init() {
  // TODO: Implement caf_corpus_init().
}

size_t afl_custom_mutator(uint8_t *data, size_t size, uint8_t *mutated_out,
                          size_t max_size, unsigned int seed) {
  auto index = *reinterpret_cast<size_t *>(data);
  auto testCase = _corpus->getTestCase(index);

  std::mt19937 rng { seed };
  caf::TestCaseMutator mutator { _corpus.get() };

  auto mutatedTestCase = mutator.mutate(testCase);
  *reinterpret_cast<size_t *>(mutated_out) = mutatedTestCase.index();

  return sizeof(size_t);
}

size_t afl_pre_save_handler(uint8_t *data, size_t size, uint8_t **new_data) {
  auto index = *reinterpret_cast<size_t *>(data);
  auto testCase = _corpus->getTestCase(index);

  caf::TestCaseSerializer serializer { };
  testcaseBuffer.clear();
  MemoryStream s { testcaseBuffer };

  serializer.write(s, *testCase);

  *new_data = testcaseBuffer.data();
  return static_cast<size_t>(testcaseBuffer.size());
}

}
