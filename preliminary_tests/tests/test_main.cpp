#include <gtest/gtest.h>

extern "C" {
  #include <transmitter.h>
}

extern "C" void _seconds_to_timespec(double seconds, struct timespec *ts) {
    ts->tv_sec = (time_t)floor(seconds);
    ts->tv_nsec = (long)((seconds - ts->tv_sec) * 1e9);
}


TEST(transmitter_create, interval_correctly_set) {
  double interval_in_secs = 1;

  struct timespec interval;
  _seconds_to_timespec(interval_in_secs, &interval);

  struct Transmitter *transmitter = transmitter_create(interval_in_secs);

  EXPECT_TRUE(transmitter->interval.tv_sec == interval.tv_sec && transmitter->interval.tv_nsec == interval.tv_nsec);
}

TEST(transmitter_send_bit, interval_is_correct) {
  double interval_in_secs = 1.5;
  double error_margin = 1.1;

  struct Transmitter *transmitter = transmitter_create(interval_in_secs);
  
  struct timespec ts_start;
  struct timespec ts_end;
  clock_gettime(CLOCK_REALTIME, &ts_start);
  transmitter_send_bit(transmitter, HIGH);
  clock_gettime(CLOCK_REALTIME, &ts_end);

  long seconds = ts_end.tv_sec - ts_start.tv_sec;
  long nanoseconds = ts_end.tv_nsec - ts_start.tv_nsec;

  if (nanoseconds < 0) {
      seconds--;
      nanoseconds += 1000000000; // 1 billion nanoseconds in a second
  }

  printf("interval  : %ld.%ld sec\n", transmitter->interval.tv_sec, transmitter->interval.tv_nsec);
  printf("Time taken: %ld.%ld sec\n", seconds, nanoseconds);
  printf("Upper limit: %ld.%ld sec\n", seconds, (long)(transmitter->interval.tv_nsec * error_margin));
  EXPECT_TRUE( (transmitter->interval.tv_sec <= seconds) && (nanoseconds  < (long)(transmitter->interval.tv_nsec*error_margin)) );
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
