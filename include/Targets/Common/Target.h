#ifndef CAF_ABSTRACT_TARGET_H
#define CAF_ABSTRACT_TARGET_H

#include "Targets/Common/ValueFactory.h"
#include "Targets/Common/AbstractExecutor.h"
#include "Targets/Common/TestCaseParser.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <utility>

namespace caf {

/**
 * @brief Represent a target.
 *
 * @tparam TargetTraits trait type describing the target's type system.
 */
template <typename TargetTraits>
class Target {
public:
  /**
   * @brief Construct a new Target object.
   *
   * @param factory the target-specific value factory.
   * @param executor the target-specific executor.
   */
  explicit Target(
      std::unique_ptr<ValueFactory<TargetTraits>> factory,
      std::unique_ptr<AbstractExecutor<TargetTraits>> executor)
    : _factory(std::move(factory)),
      _executor(std::move(executor))
  {
    assert(_factory && "factory cannot be null.");
    assert(_executor && "executor cannot be null.");
  }

  /**
   * @brief Get the target-specific factory.
   *
   * @return ValueFactory<TargetTraits>* the target-specific factory.
   */
  ValueFactory<TargetTraits>& factory() const { return *_factory; }

  /**
   * @brief Get the target-specific executor.
   *
   * @return AbstractExecutor<TargetTraits>& the target-specific executor.
   */
  AbstractExecutor<TargetTraits>& executor() const { return *_executor; }

  /**
   * @brief Run the target.
   *
   */
  void Run() {
    StlInputStream input { std::cin };
    TestCaseParser<TargetTraits> parser { input, factory(), executor() };
    parser.ParseAndRun();
  }

private:
  std::unique_ptr<ValueFactory<TargetTraits>> _factory;
  std::unique_ptr<AbstractExecutor<TargetTraits>> _executor;
}; // class Target

} // namespace caf

#endif
