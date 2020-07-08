#ifndef CAF_PROBE_EXECUTOR_H
#define CAF_PROBE_EXECUTOR_H

namespace caf {

class TestCase;

/**
 * @brief Abstract base class for probe executors.
 *
 */
class ProbeExecutor {
public:
  /**
   * @brief Execute the given test case and report whether the target program fails.
   *
   * @param tc The test case to execute.
   * @return true if the target program does not fail on the given test case.
   * @return false if the target program fail on the given test case.
   */
  virtual bool Execute(const TestCase& tc) = 0;
}; // class ProbeExecutor

} // namespace caf

#endif
