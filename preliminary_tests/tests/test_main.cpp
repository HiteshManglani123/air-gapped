#include <gtest/gtest.h>

extern "C" {
  #include <transmitter.h>
}

TEST(transmitter_create, interval_correctly_set) {
  int interval = 1 * CLOCKS_PER_SEC;
  struct Transmitter *transmitter = transmitter_create(interval);

  EXPECT_EQ(transmitter->interval, interval);
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}