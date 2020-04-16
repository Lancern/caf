#include "gtest/gtest.h"
#include "Infrastructure/Memory.h"
#include "Infrastructure/Random.h"
#include "Basic/CAFStore.h"
#include "Basic/Function.h"
#include "Fuzzer/TestCase.h"
#include "Fuzzer/TestCaseGenerator.h"
#include "Fuzzer/ObjectPool.h"
#include "Fuzzer/Value.h"

#include <memory>
#include <unordered_set>
#include <queue>

namespace {

std::unique_ptr<caf::CAFStore> CreateMockStore() {
  auto store = caf::make_unique<caf::CAFStore>();
  store->AddFunction(caf::Function { 0, "func" });
  return store;
}

void AssertNoXref(const caf::TestCase& tc) {
  std::queue<const caf::Value *> que;
  for (const auto& func : tc) {
    if (func.HasThis()) {
      que.push(func.GetThis());
    }
    for (auto arg : func) {
      que.push(arg);
    }
  }

  std::unordered_set<const caf::Value *> visited;
  while (!que.empty()) {
    auto curr = que.front();
    que.pop();

    if (!curr->IsArray()) {
      continue;
    }

    if (visited.find(curr) != visited.end()) {
      testing::AssertionFailure();
    }

    auto arrayValue = caf::dyn_cast<caf::ArrayValue>(curr);
    for (size_t i = 0; i < arrayValue->size(); ++i) {
      auto element = arrayValue->GetElement(i);
      que.push(element);
    }
  }
}

void AssertNoPlaceholder(const caf::Value* value) {
  std::unordered_set<const caf::Value *> visited { value };
  std::queue<const caf::Value *> que;
  que.push(value);

  while (!que.empty()) {
    value = que.front();
    que.pop();

    ASSERT_NE(value->kind(), caf::ValueKind::Placeholder);

    if (!value->IsArray()) {
      continue;
    }

    auto arrayValue = caf::dyn_cast<caf::ArrayValue>(value);
    for (size_t i = 0; i < arrayValue->size(); ++i) {
      auto element = arrayValue->GetElement(i);
      if (visited.find(element) == visited.end()) {
        visited.insert(element);
        que.push(element);
      }
    }
  }
}

} // namespace <anonymous>

TEST(TestCaseGenerator, GenerateTestCaseNoXref) {
  auto store = CreateMockStore();
  auto pool = caf::make_unique<caf::ObjectPool>();
  caf::Random<> rnd;
  for (int round = 0; round < 1000000; ++round) {
    caf::TestCaseGenerator gen { *store, *pool, rnd };
    auto tc = gen.GenerateTestCase(0);
    AssertNoXref(tc);
  }
}

TEST(TestCaseGenerator, GenerateValueNoPlaceholder) {
  auto store = CreateMockStore();
  auto pool = caf::make_unique<caf::ObjectPool>();
  caf::Random<> rnd;
  for (int round = 0; round < 1000000; ++round) {
    caf::TestCaseGenerator gen { *store, *pool, rnd };
    caf::TestCaseGenerator::GeneratePlaceholderValueParams params;
    auto value = gen.GenerateValue(0, params);
    AssertNoPlaceholder(value);
  }
}
