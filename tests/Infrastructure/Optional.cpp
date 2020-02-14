#include "gtest/gtest.h"
#include "Infrastructure/Optional.h"

TEST(Optional, take) {
  std::vector<int> x { 1, 2, 3, 4, 5 };
  caf::Optional<std::vector<int>> opt { x };
  // ASSERT_EQ(x, opt.take());
}
