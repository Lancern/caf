#ifndef CAF_CORPUS_H
#define CAF_CORPUS_H

#include "Infrastructure/Random.h"
#include "Fuzzer/TestCase.h"
#include "Fuzzer/ObjectPool.h"

#include <cassert>
#include <utility>
#include <vector>

namespace caf {

class CAFStore;
class Corpus;

/**
 * @brief A reference to a test case managed by a corpus.
 *
 */
class TestCaseRef {
public:
  /**
   * @brief Construct a new TestCaseRef object that represents an empty reference.
   *
   */
  explicit TestCaseRef()
    : _corpus(nullptr),
      _index(0)
  { }

  /**
   * @brief Construct a new TestCaseRef object.
   *
   * @param corpus the corpus.
   * @param index the index of the test case in the corpus.
   */
  explicit TestCaseRef(Corpus* corpus, size_t index)
    : _corpus(corpus),
      _index(index)
  { }

  /**
   * @brief Determine whether the reference is empty.
   *
   * @return true if the reference is empty.
   * @return false if the reference is not empty.
   */
  bool empty() const { return _corpus == nullptr; }

  operator bool() const { return empty(); }

  /**
   * @brief Get the index of the referenced test case inside the corpus.
   *
   * @return size_t the index.
   */
  size_t index() const {
    assert(!empty() && "Trying to dereference an empty TestCaseRef.");
    return _index;
  }

  /**
   * @brief Get a pointer to the referenced test case.
   *
   * @return TestCase* a pointer to the referenced test case.
   */
  inline TestCase* get() const;

  TestCase& operator*() const {
    assert(!empty() && "Trying to dereference an empty TestCaseRef.");
    return *get();
  }

  TestCase* operator->() const {
    assert(!empty() && "Trying to dereference an empty TestCaseRef.");
    return get();
  }

private:
  Corpus* _corpus;
  size_t _index;
};

/**
 * @brief Test case corpus.
 *
 */
class Corpus {
public:
  /**
   * @brief Construct a new Corpus object.
   *
   * @param store the metadata store.
   */
  explicit Corpus(std::unique_ptr<CAFStore> store);

  Corpus(const Corpus &) = delete;
  Corpus(Corpus &&) noexcept;

  Corpus& operator=(const Corpus &) = delete;
  Corpus& operator=(Corpus &&);

  ~Corpus();

  /**
   * @brief Get the metadata store.
   *
   * @return CAFStore* the metadata store.
   */
  CAFStore* store() const { return _store.get(); }

  /**
   * @brief Get the object pool contained in this corpus.
   *
   * @return ObjectPool& the object pool.
   */
  ObjectPool& pool() { return _pool; }

  /**
   * @brief Get the object pool contained in this corpus.
   *
   * @return ObjectPool& the object pool.
   */
  const ObjectPool& pool() const { return _pool; }

  /**
   * @brief Create a new test case managed by this corpus.
   *
   * @return TestCase* the test case created.
   */
  TestCaseRef CreateTestCase();

  /**
   * @brief Duplicate the given test case.
   *
   * @param existing the existing test case to duplicate.
   * @return TestCaseRef the duplicated test case.
   */
  TestCaseRef DuplicateTestCase(TestCaseRef existing);

  /**
   * @brief Add the given test case to this corpus.
   *
   * @param testCase the test case.
   */
  TestCaseRef AddTestCase(TestCase testCase);

  /**
   * @brief Get the number of test cases inside this corpus.
   *
   * @return size_t the number of test cases inside this corpus.
   */
  size_t GetTestCasesCount() const { return _testCases.size(); }

  /**
   * @brief Get the test case at the given index.
   *
   * @param index the index.
   * @return TestCase* the test case at the given index.
   */
  TestCaseRef GetTestCase(size_t index) {
    assert(index >= 0 && index < _testCases.size() && "index is out of range.");
    return TestCaseRef { this, index };
  }

  template <typename T>
  TestCaseRef SelectTestCase(Random<T>& rnd) {
    size_t index = rnd.Index(_testCases);
    return TestCaseRef { this, index };
  }

  friend class TestCaseRef;

private:
  std::unique_ptr<CAFStore> _store;
  std::vector<TestCase> _testCases;
  ObjectPool _pool;
}; // class Corpus

TestCase* TestCaseRef::get() const {
  if (_corpus) {
    return &_corpus->_testCases.at(_index);
  } else {
    return nullptr;
  }
}

} // namespace caf

#endif
