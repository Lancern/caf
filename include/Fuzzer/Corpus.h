#ifndef CAF_CORPUS_H
#define CAF_CORPUS_H

#include "Infrastructure/Memory.h"
#include "Infrastructure/Random.h"
#include "Fuzzer/ObjectPool.h"
#include "Fuzzer/TestCase.h"

#include <memory>
#include <vector>
#include <unordered_map>

namespace caf {

class CAFStore;
class CAFCorpus;

/**
 * @brief Provide a smart pointer type that references to a test case owned by a `CAFCorpus`
 * instance.
 *
 */
class CAFCorpusTestCaseRef {
public:
  /**
   * @brief Construct a new CAFCorpusTestCaseRef object that represents an empty reference.
   *
   */
  explicit CAFCorpusTestCaseRef()
    : _corpus(nullptr),
      _index(0)
  { }

  /**
   * @brief Construct a new CAFCorpusTestCaseRef object.
   *
   * @param corpus the corpus owning the referenced `TestCase` object.
   * @param index the index of the @see TestCase object inside the corpus.
   */
  explicit CAFCorpusTestCaseRef(CAFCorpus* corpus, size_t index)
    : _corpus(corpus),
      _index(index)
  { }

  /**
   * @brief Get the corpus instance owning the test case.
   *
   * @return CAFCorpus* the corpus instance owning the test case.
   */
  CAFCorpus* corpus() const { return _corpus; }

  /**
   * @brief Get the index of the referenced `TestCase` instance inside the corpus.
   *
   * @return index_type the index of the referenced `TestCase` instance.
   */
  size_t index() const { return _index; }

  /**
   * @brief Determine whether this reference is valid, a.k.a. it actually references to a valid
   * object.
   *
   * @return true if this reference is valid.
   * @return false if this reference is not valid.
   */
  bool valid() const { return static_cast<bool>(_corpus); }

  operator bool() const { return valid(); }

  TestCase& operator*() const { return *get(); }

  TestCase* operator->() const { return get(); }

  /**
   * @brief Get a raw pointer to the actual @see TestCase object pointed-to by this
   * @see CAFCorpusTestCaseRef object.
   *
   * @return TestCase*
   */
  inline TestCase* get() const;

private:
  CAFCorpus* _corpus;
  size_t _index;
};

/**
 * @brief Corpus of the fuzzer. This is the top-level object maintained in the fuzzer server.
 *
 * Instance of corpus contains two parts: a @see CAFStore object and a bunch of @see CAFObjectPool
 * objects. Each @see CAFObjectPool object is associated with the ID of the type of the objects
 * contained in that @see CAFObjtecPool.
 *
 */
class CAFCorpus {
public:
  /**
   * @brief Construct a new CAFCorpus object.
   *
   * @param store the `CAFStore` instance containing metadata about the target program's type
   * information.
   */
  explicit CAFCorpus(std::unique_ptr<CAFStore> store)
    : _store(std::move(store)),
      _pools { }
  { }

  ~CAFCorpus();

  /**
   * @brief Get the @see CAFStore instance containing type information and API information in the
   * target program.
   *
   * @return CAFStore* the @see CAFStore instance containing type information and API information in
   * the target program.
   */
  CAFStore* store() const { return _store.get(); }

  /**
   * @brief Get a list of test cases contained in the corpus.
   *
   * @return const std::vector<TestCase> & a list of test cases contained in the corpus.
   */
  const std::vector<TestCase>& testCases() const { return _testCases; }

  /**
   * @brief Get the @see TestCase object at the given index.
   *
   * @param index the index of the desired `TestCase` object.
   * @return CAFCorpusTestCaseRef smart pointer to the test case object.
   */
  CAFCorpusTestCaseRef GetTestCase(size_t index) {
    return CAFCorpusTestCaseRef { this, index };
  }

  /**
   * @brief Get a raw pointer to the @see TestCase object at the given index. Note that this pointer
   * might be invalidated after new test cases are added to this @see CAFCorpus.
   *
   * @param index the index of the test case.
   * @return TestCase* a raw pointer to the test case.
   */
  TestCase* GetTestCaseRaw(size_t index) {
    return &_testCases[index];
  }

  /**
   * @brief Create a new test case.
   *
   * @tparam Args types of arguments used to construct a new `TestCase` object.
   * @param args arguments used to construct a new `TestCase` object.
   * @return CAFCorpusTestCaseRef a `CAFCorpusTestCaseRef` instance representing a smart pointer
   * to the created `TestCase` object.
   */
  template <typename ...Args>
  CAFCorpusTestCaseRef CreateTestCase(Args&&... args) {
    _testCases.emplace_back(std::forward<Args>(args)...);
    return CAFCorpusTestCaseRef { this, _testCases.size() - 1 };
  }

  /**
   * @brief Get the object pool associated with the given type ID.
   *
   * @param typeId the type ID of the object pool.
   * @return CAFObjectPool* pointer to the desired object pool. If no such object pool is found,
   * `nullptr` will be returned.
   */
  CAFObjectPool* GetObjectPool(uint64_t typeId) const;

  /**
   * @brief Get the object pool associated with the given type name. If the desired object pool does
   * not exist, then create it.
   *
   * @param typeId the type ID of the object pool.
   * @return CAFObjectPool* pointer to the object pool.
   */
  CAFObjectPool* GetOrCreateObjectPool(uint64_t typeId);

  /**
   * @brief Get a special object pool that owns all placeholder values.
   *
   * @return CAFObjectPool* pointer to a special object pool that owns all placeholder values.
   */
  CAFObjectPool* GetPlaceholderObjectPool();

  /**
   * @brief Randomly select a test case from this corpus, using the given random number generator.
   *
   * @tparam RNG the type of the random number generator.
   * @param rnd the random number genreator to use.
   * @return CAFCorpusTestCaseRef the selected test case.
   */
  template <typename RNG>
  CAFCorpusTestCaseRef SelectTestCase(Random<RNG>& rnd) {
    return CAFCorpusTestCaseRef { this, rnd.Index(_testCases) };
  }

private:
  std::unique_ptr<CAFStore> _store;
  std::unordered_map<uint64_t, std::unique_ptr<CAFObjectPool>> _pools;
  std::unique_ptr<CAFObjectPool> _placeholderPool;
  std::vector<TestCase> _testCases;
};

inline TestCase* CAFCorpusTestCaseRef::get() const {
  return _corpus->GetTestCaseRaw(_index);
}

} // namespace caf

#endif
