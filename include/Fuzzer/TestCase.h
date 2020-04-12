#ifndef CAF_TEST_CASE_H
#define CAF_TEST_CASE_H

#include "Infrastructure/Random.h"
#include "Fuzzer/FunctionCall.h"

#include <cassert>
#include <utility>
#include <iterator>
#include <vector>

namespace caf {

/**
 * @brief A test case.
 *
 */
class TestCase {
public:
  using Iterator = typename std::vector<FunctionCall>::iterator;
  using ConstIterator = typename std::vector<FunctionCall>::const_iterator;

  /**
   * @brief Construct a new TestCase object.
   *
   */
  explicit TestCase()
    : _storeRootEntryIndex(0),
      _calls()
  { }

  /**
   * @brief Get the index of the root entry in the CAF metadata store from which the function calls
   * in this test case are selected.
   *
   * @return size_t the index of the root entry.
   */
  size_t storeRootEntryIndex() const { return _storeRootEntryIndex; }

  /**
   * @brief Set the index of the root entry.
   *
   * @param storeRootEntryIndex the index of the root entry.
   */
  void SetStoreRootEntryIndex(size_t storeRootEntryIndex) {
    _storeRootEntryIndex = storeRootEntryIndex;
  }

  /**
   * @brief Get function calls included in this test case.
   *
   * @return const std::vector<FunctionCall>& function calls included in this test case.
   */
  const std::vector<FunctionCall>& calls() const { return _calls; }

  /**
   * @brief Get the number of function calls.
   *
   * @return size_t the number of function calls.
   */
  size_t GetFunctionCallsCount() const { return _calls.size(); }

  /**
   * @brief Get the function call at the given index.
   *
   * @param index the index.
   * @return FunctionCall& the function call at the given index.
   */
  FunctionCall& GetFunctionCall(size_t index) { return _calls.at(index); }

  /**
   * @brief Get the function call at the given index.
   *
   * @param index the index.
   * @return const FunctionCall& the function call at the given index.
   */
  const FunctionCall& GetFunctionCall(size_t index) const { return _calls.at(index); }

  FunctionCall& operator[](size_t index) { return GetFunctionCall(index); }

  const FunctionCall& operator[](size_t index) const { return GetFunctionCall(index); }

  /**
   * @brief Set the function call at the given index.
   *
   * @param index the index.
   * @param call the function call.
   */
  void SetFunctionCall(size_t index, FunctionCall call) {
    _calls.at(index) = std::move(call);
  }

  /**
   * @brief Reserve at least the given capacity for the function call sequence.
   *
   * @param capacity the minimum capacity.
   */
  void ReserveFunctionCalls(size_t capacity) { _calls.reserve(capacity); }

  /**
   * @brief Add a new function call to this test case.
   *
   * @param call the function call to add.
   */
  void PushFunctionCall(FunctionCall call) { _calls.push_back(std::move(call)); }

  /**
   * @brief Insert the given function call at the given index.
   *
   * @param index the index of the function call.
   * @param call the function call to be inserted.
   */
  void InsertFunctionCall(size_t index, FunctionCall call) {
    assert(index >= 0 && index <= _calls.size() && "Index is out of range.");
    _calls.insert(std::next(_calls.begin(), index), std::move(call));
  }

  /**
   * @brief Remove the function call at the given index.
   *
   * @param index the index of the function call.
   */
  void RemoveFunctionCall(size_t index) {
    assert(index >= 0 && index < _calls.size() && "Index is out of range.");
    _calls.erase(std::next(_calls.begin(), index));
  }

  /**
   * @brief Remove all function calls starting at the given index.
   *
   * @param startIndex the start index.
   */
  void RemoveTailCalls(size_t startIndex) {
    assert(startIndex >= 0 && startIndex <= _calls.size() && "Index is out of range.");
    _calls.erase(std::next(_calls.begin(), startIndex), _calls.end());
  }

  /**
   * @brief Add all function calls given in the list to the tail of this test case.
   *
   * @param calls the function calls to add.
   */
  void AppendFunctionCalls(std::vector<FunctionCall> calls) {
    for (auto& call : calls) {
      _calls.push_back(std::move(call));
    }
  }

  /**
   * @brief Randomly select a function call from this test case, using the given random number
   * generator.
   *
   * @tparam T type of the random bit generator.
   * @param rnd the random number generator.
   * @return FunctionCall& the selected function call.
   */
  template <typename T>
  FunctionCall& SelectFunctionCall(Random<T>& rnd) {
    return rnd.Select(_calls);
  }

  Iterator begin() { return _calls.begin(); }

  Iterator end() { return _calls.end(); }

  ConstIterator begin() const { return _calls.begin(); }

  ConstIterator end() const { return _calls.end(); }

private:
  size_t _storeRootEntryIndex;
  std::vector<FunctionCall> _calls;
}; // class TestCase

} // namespace caf

#endif
