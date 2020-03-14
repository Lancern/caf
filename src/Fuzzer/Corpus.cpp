#include "Infrastructure/Memory.h"
#include "Basic/CAFStore.h"
#include "Fuzzer/Corpus.h"

namespace caf {

Corpus::Corpus(std::unique_ptr<CAFStore> store)
  : _store(std::move(store)),
    _testCases(),
    _pool { }
{ }

Corpus::Corpus(Corpus &&) noexcept = default;

Corpus& Corpus::operator=(Corpus &&) = default;

Corpus::~Corpus() = default;

TestCaseRef Corpus::CreateTestCase() {
  AddTestCase(TestCase { });
  return TestCaseRef { this, _testCases.size() - 1 };
}

TestCaseRef Corpus::DuplicateTestCase(TestCaseRef existing) {
  auto dup = *existing;
  return AddTestCase(std::move(dup));
}

TestCaseRef Corpus::AddTestCase(TestCase testCase) {
  size_t index = _testCases.size();
  _testCases.push_back(std::move(testCase));
  return TestCaseRef { this, index };
}

} // namespace caf
