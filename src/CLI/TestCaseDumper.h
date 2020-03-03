#ifndef CAF_TEST_CASE_DUMPER_H
#define CAF_TEST_CASE_DUMPER_H

namespace caf {

class TestCase;

/**
 * @brief Provide methods to dump test cases.
 *
 */
class TestCaseDumper {
public:
  /**
   * @brief Dump the given test case to standard output stream.
   *
   * @param tc the test case to dump.
   */
  void Dump(const TestCase& tc);
}; // class TestCaseDumper

} // namespace caf

#endif
