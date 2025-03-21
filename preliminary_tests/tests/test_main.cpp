// tests/test_main.cpp
#include <gtest/gtest.h>

// Example test case
TEST(Example_Test, interval_correctly_set) {
  EXPECT_EQ(7 * 6, 42);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
