#ifndef CAF_TEST_CASE_GENERATOR_H
#define CAF_TEST_CASE_GENERATOR_H

#include "Infrastructure/Random.h"
#include "Fuzzer/Value.h"

#include <cassert>
#include <memory>

namespace caf {

class CAFStore;
class ObjectPool;
class TestCase;
class FunctionCall;

/**
 * @brief Generate new test cases and values.
 *
 */
class TestCaseGenerator {
public:
  /**
   * @brief Options for the generator.
   *
   */
  struct Options {
    explicit Options()
      : MaxCalls(5),
        MaxStringLength(10),
        MaxArrayLength(5),
        MaxArguments(5),
        MaxDepth(3)
    { }

    size_t MaxCalls; // Maximum number of calls in generated test case.
    size_t MaxStringLength; // Maximum length of generated string values.
    size_t MaxArrayLength; // Maximum length of generated array values.
    size_t MaxArguments; // Maximum number of arguments to generate for a function call.
    size_t MaxDepth; // Maximum numbers of levels in the generated value.
  };

  /**
   * @brief Provide parameters for generating placeholder values.
   *
   */
  class GeneratePlaceholderValueParams {
  public:
    /**
     * @brief Construct a new GeneratePlaceholderValueParams object.
     *
     */
    explicit GeneratePlaceholderValueParams()
      : _currCallIndex(0)
    { }

    /**
     * @brief Construct a new GeneratePlaceholderValueParams object.
     *
     * @param currCallIndex the current function call's index.
     */
    explicit GeneratePlaceholderValueParams(size_t currCallIndex)
      : _currCallIndex(currCallIndex)
    { }

    /**
     * @brief Determine whether the generator should generate a placeholder value.
     *
     * @return true if the generator should generate a placeholder value.
     * @return false if the generator shouldd not generate a placeholder value.
     */
    bool ShouldGenerate() const { return _currCallIndex != 0; }

    /**
     * @brief Get the current function call's index.
     *
     * @return int the current function call's index. This function returns -1 if the current object
     * instructs the generator not to generate a placeholder value.
     */
    size_t GetCurrentCallIndex() const { return _currCallIndex; }

    /**
     * @brief Set the current function call's index.
     *
     * @param index the current function call's index.
     */
    void SetCurrentCallIndex(size_t index) { _currCallIndex = index; }

  private:
    size_t _currCallIndex;
  };

  /**
   * @brief Construct a new TestCaseGenerator object.
   *
   * @param store the CAF metadata store.
   * @param pool the object pool.
   * @param rnd the random number generator.
   */
  explicit TestCaseGenerator(CAFStore& store, ObjectPool& pool, Random<>& rnd)
    : _store(store),
      _pool(pool),
      _rnd(rnd),
      _opt()
  { }

  TestCaseGenerator(const TestCaseGenerator &) = delete;
  TestCaseGenerator(TestCaseGenerator &&) noexcept = default;

  /**
   * @brief Get the options.
   *
   * @return Options& the options.
   */
  Options& options() { return _opt; }

  /**
   * @brief Get the options.
   *
   * @return const Options& the options.
   */
  const Options& options() const { return _opt; }

  /**
   * @brief Generate a new test case.
   *
   * @return TestCase the test case generated.
   */
  TestCase GenerateTestCase();

  /**
   * @brief Generate a new function call.
   *
   * @param index the index of the function call to be generated.
   * @param rootEntryIndex the index of the root entry from which the callee function will be
   * selected.
   * @return FunctionCall the function call generated.
   */
  FunctionCall GenerateFunctionCall(size_t index, size_t rootEntryIndex);

  /**
   * @brief Generate a new value.
   *
   * @param rootEntryIndex the index of the root entry from which callee functions of function
   * values will be selected.
   * @param params the parameters for generating placeholder values.
   * @return Value* the value generated.
   */
  Value* GenerateValue(
      size_t rootEntryIndex,
      GeneratePlaceholderValueParams params = GeneratePlaceholderValueParams()) {
    return GenerateValue(rootEntryIndex, params, 1);
  }

  /**
   * @brief Generate a function value.
   *
   * @param rootEntryIndex the index of the root entry from which the callee function will be
   * selected.
   * @return FunctionValue* the generated function value.
   */
  FunctionValue* GenerateFunctionValue(size_t rootEntryIndex);

  /**
   * @brief Generate a char that can be added to a string value.
   *
   * @return char the generated character.
   */
  char GenerateStringCharacter();

private:
  CAFStore& _store;
  ObjectPool& _pool;
  Random<>& _rnd;
  Options _opt;

  /**
   * @brief Randomly generate a number indicating how many arguments should be generated for a
   * function call.
   *
   * @return size_t the number of arguments to generate.
   */
  size_t GenerateArgumentsCount();

  /**
   * @brief Generate a ValueKind value.
   *
   * @param generateArrayKind should we generate ArrayKind?
   * @param generatePlaceholderKind should we generate PlaceholderKind?
   * @return ValueKind the generated value.
   */
  ValueKind GenerateValueKind(bool generateArrayKind, bool generatePlaceholderKind);

  /**
   * @brief Generate a new value.
   *
   * @param rootEntryIndex the index of the root entry from which callee functions of function
   * values will be selected.
   * @param params the parameters for generating placeholder values.
   * @param depth depth of the current genreation process.
   * @return Value* the generated value.
   */
  Value* GenerateValue(size_t rootEntryIndex, GeneratePlaceholderValueParams params, size_t depth);
}; // class TestCaseGenerator

} // namespace caf

#endif
