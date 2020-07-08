#ifndef CAF_NODE_EXECUTOR_H
#define CAF_NODE_EXECUTOR_H

#include "Prober/ProbeExecutor.h"

namespace caf {

class TestCase;

/**
 * @brief Provide implementation of ProbeExecutor to execute on nodejs.
 *
 */
class NodeExecutor : public ProbeExecutor {
public:
  virtual bool Execute(const TestCase& tc) override;
}; // class NodeExecutor

} // namespace caf

#endif
