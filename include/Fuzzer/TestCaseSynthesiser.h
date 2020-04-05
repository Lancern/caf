#ifndef CAF_TEST_CASE_SYNTHESISER_H
#define CAF_TEST_CASE_SYNTHESISER_H

#include <string>

namespace caf {

class CAFStore;
class TestCase;
class SynthesisBuilder;

/**
 * @brief Synthesis test cases to equivalent JavaScript code.
 *
 */
class TestCaseSynthesiser {
public:
  /**
   * @brief Construct a new TestCaseSynthesiser object.
   *
   * @param store the CAF metadata store.
   * @param builder the synthesis builder.
   */
  explicit TestCaseSynthesiser(const CAFStore& store, SynthesisBuilder& builder)
    : _store(store), _builder(builder)
  { }

  /**
   * @brief Get the CAF metadata store.
   *
   * @return const CAFStore& the CAF metadata store.
   */
  const CAFStore& store() const { return _store; }

  /**
   * @brief Synthesis the given test case.
   *
   * @param tc the test case to synthesis.
   */
  void Synthesis(const TestCase& tc);

  /**
   * @brief Get the synthesised code.
   *
   * @return const std::string& the synthesised code.
   */
  const std::string& GetCode() const;

private:
  const CAFStore& _store;
  SynthesisBuilder& _builder;
}; // class TestCaseSynthesiser

} // namespace caf

#endif
