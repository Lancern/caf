#include "gtest/gtest.h"
#include "Infrastructure/Optional.h"

#include <utility>
#include <vector>
#include <string>

TEST(Optional, ConstructEmpty) {
  caf::Optional<std::vector<int>> opt { };
  ASSERT_FALSE(opt.hasValue());
}

TEST(Optional, ConstructNonEmpty) {
  std::vector<int> x { 1, 2, 3, 4, 5 };
  caf::Optional<std::vector<int>> opt { std::vector<int> { 1, 2, 3, 4, 5 } };
  ASSERT_TRUE(opt.hasValue());
  ASSERT_EQ(x, opt.value());
}

TEST(Optional, CopyConstructEmpty) {
  caf::Optional<std::vector<int>> a { };
  caf::Optional<std::vector<int>> b { a };
  ASSERT_FALSE(b.hasValue());
}

TEST(Optional, CopyConstructNonEmpty) {
  std::vector<int> x { 1, 2, 3, 4, 5 };
  caf::Optional<std::vector<int>> a { x };
  caf::Optional<std::vector<int>> b { a };
  ASSERT_TRUE(b.hasValue());
  ASSERT_EQ(x, b.value());
}

TEST(Optional, MoveConstructEmpty) {
  caf::Optional<std::vector<int>> a { };
  caf::Optional<std::vector<int>> b { std::move(a) };
  ASSERT_FALSE(b.hasValue());
}

TEST(Optional, MoveConstructNonEmpty) {
  std::vector<int> x { 1, 2, 3, 4, 5 };
  caf::Optional<std::vector<int>> a { x };
  caf::Optional<std::vector<int>> b { std::move(a) };
  ASSERT_FALSE(a.hasValue());
  ASSERT_TRUE(b.hasValue());
  ASSERT_EQ(x, b.value());
}

TEST(Optional, CopyAssignmentEmpty) {
  caf::Optional<std::vector<int>> a { };
  caf::Optional<std::vector<int>> b;
  b = a;
  ASSERT_FALSE(b.hasValue());
}

TEST(Optional, CopyAssignmentNonEmpty) {
  std::vector<int> x { 1, 2, 3, 4, 5 };
  caf::Optional<std::vector<int>> a { x };
  caf::Optional<std::vector<int>> b;
  b = a;
  ASSERT_TRUE(b.hasValue());
  ASSERT_EQ(x, b.value());
}

TEST(Optional, MoveAssignmentEmpty) {
  caf::Optional<std::vector<int>> a { };
  caf::Optional<std::vector<int>> b;
  b = std::move(a);
  ASSERT_FALSE(b.hasValue());
}

TEST(Optional, MoveAssignmentNonEmpty) {
  std::vector<int> x { 1, 2, 3, 4, 5 };
  caf::Optional<std::vector<int>> a { x };
  caf::Optional<std::vector<int>> b;
  b = std::move(a);
  ASSERT_FALSE(a.hasValue());
  ASSERT_TRUE(b.hasValue());
  ASSERT_EQ(x, b.value());
}

TEST(Optional, emplace) {
  std::string s = "abc";
  caf::Optional<std::string> opt { };
  opt.emplace("abc");
  ASSERT_TRUE(opt.hasValue());
  ASSERT_EQ(s, opt.value());
}

TEST(Optional, set) {
  std::string s = "abc";
  caf::Optional<std::string> opt { };
  opt.set(s);
  ASSERT_TRUE(opt.hasValue());
  ASSERT_EQ(s, opt.value());
}

TEST(Optional, take) {
  std::vector<int> x { 1, 2, 3, 4, 5 };
  caf::Optional<std::vector<int>> opt { x };
  ASSERT_EQ(x, opt.take());
  ASSERT_FALSE(opt.hasValue());
}

namespace {

caf::Optional<std::vector<int>> CreateLotsOfInt(bool create, int count) {
  if (!create) {
    return caf::Optional<std::vector<int>> { };
  }

  std::vector<int> values;
  if (count > 0) {
    values.reserve(count);
  }

  for (auto i = 0; i < count; ++i) {
    values.push_back(i);
  }

  return caf::Optional<std::vector<int>> { std::move(values) };
}

} // namespace <anonymous>

TEST(Optional, ReturnMe) {
  auto values = CreateLotsOfInt(true, 100).take();
  ASSERT_EQ(100, values.size());
  for (size_t i = 0; i < values.size(); ++i) {
    ASSERT_EQ(i, values[i]);
  }
}
