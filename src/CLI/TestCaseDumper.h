#ifndef CAF_TEST_CASE_DUMPER_H
#define CAF_TEST_CASE_DUMPER_H

#include "Printer.h"

namespace caf {

class TestCase;

/**
 * @brief Provide methods to dump test cases.
 *
 */
class TestCaseDumper {
public:
  /**
   * @brief Construct a new TestCaseDumper object.
   *
   * @param printer the printer.
   */
  explicit TestCaseDumper(Printer& printer)
    : _printer { printer }
  { }

  /**
   * @brief Dump the given test case to standard output stream.
   *
   * @param tc the test case to dump.
   */
  void Dump(const TestCase& tc);

private:
  Printer& _printer;
}; // class TestCaseDumper

} // namespace caf

#endif
