#include <gtest/gtest.h>

extern "C" {
  #include <transmitter.h>
}

extern "C" void _seconds_to_timespec(double seconds, struct timespec *ts) {
    ts->tv_sec = (time_t)floor(seconds);
    ts->tv_nsec = (long)((seconds - ts->tv_sec) * 1e9);
}

class TransmitterFixture : public testing::Test
{
public:
  TransmitterFixture(){
    double interval_in_secs = 1.3;
    transmitter = transmitter_create(interval_in_secs);
  }
  ~TransmitterFixture(){}
  void SetUp(){}
  void TearDown(){}

  struct Transmitter *transmitter;

  double higher_error_margin = 1.1;
  double lower_error_margin = 0.9;
};

TEST_F(TransmitterFixture, high_bit_transmission_time_is_within_expected_error_margin) {
  // Get the expected time interval for transmission
  struct timespec interval = transmitter_get_interval(transmitter);
  
  struct timespec ts_start, ts_end;
  clock_gettime(CLOCK_REALTIME, &ts_start);
  
  // Simulate transmitting a HIGH bit
  transmitter_send_bit(transmitter, HIGH);
  
  clock_gettime(CLOCK_REALTIME, &ts_end);
  
  // Calculate elapsed time in seconds and nanoseconds
  long seconds = ts_end.tv_sec - ts_start.tv_sec;
  long nanoseconds = ts_end.tv_nsec - ts_start.tv_nsec;

  // Handle the case where nanoseconds might be negative
  if (nanoseconds < 0) {
      seconds--;
      nanoseconds += 1000000000; // Adjust nanoseconds if negative
  }

  // Calculate lower and upper bounds based on the expected interval and error margin
  long lower_limit_sec = seconds;
  long lower_limit_nsec = (long)(interval.tv_nsec * lower_error_margin);
  long upper_limit_sec = seconds;
  long upper_limit_nsec = (long)(interval.tv_nsec * higher_error_margin);

  // Print debug information (optional)
  printf("Expected interval     : %ld.%ld sec\n", interval.tv_sec, interval.tv_nsec);
  printf("Lower limit           : %ld.%ld sec\n", lower_limit_sec, lower_limit_nsec);
  printf("Transmission time     : %ld.%ld sec\n", seconds, nanoseconds);
  printf("Upper limit           : %ld.%ld sec\n", upper_limit_sec, upper_limit_nsec);

  // Assert that the transmission time is within the expected bounds
  EXPECT_TRUE(
    (seconds > lower_limit_sec || (seconds == lower_limit_sec && nanoseconds >= lower_limit_nsec)) &&
    (seconds < upper_limit_sec || (seconds == upper_limit_sec && nanoseconds <= upper_limit_nsec))
  ) << "Time taken to transmit the high bit is out of the boundaries of the error margin";
}

TEST_F(TransmitterFixture, low_bit_transmission_time_is_within_expected_error_margin) {
  // Get the expected time interval for transmission
  struct timespec interval = transmitter_get_interval(transmitter);

  struct timespec ts_start, ts_end;
  clock_gettime(CLOCK_REALTIME, &ts_start);

  // Simulate transmitting a LOW bit
  transmitter_send_bit(transmitter, LOW);

  clock_gettime(CLOCK_REALTIME, &ts_end);

  // Calculate elapsed time in seconds and nanoseconds
  long seconds = ts_end.tv_sec - ts_start.tv_sec;
  long nanoseconds = ts_end.tv_nsec - ts_start.tv_nsec;

  // Handle the case where nanoseconds might be negative
  if (nanoseconds < 0) {
      seconds--;
      nanoseconds += 1000000000; // Adjust nanoseconds if negative
  }

  // Calculate lower and upper bounds based on the expected interval and error margin
  long lower_limit_sec = seconds;
  long lower_limit_nsec = (long)(interval.tv_nsec * lower_error_margin);
  long upper_limit_sec = seconds;
  long upper_limit_nsec = (long)(interval.tv_nsec * higher_error_margin);

  // Print debug information (optional)
  // You can keep this for debugging purposes, but ideally, avoid using printf
  // in unit tests or replace it with appropriate logging mechanisms.
  printf("Expected interval     : %ld.%ld sec\n", interval.tv_sec, interval.tv_nsec);
  printf("Lower limit           : %ld.%ld sec\n", lower_limit_sec, lower_limit_nsec);
  printf("Transmission time     : %ld.%ld sec\n", seconds, nanoseconds);
  printf("Upper limit           : %ld.%ld sec\n", upper_limit_sec, upper_limit_nsec);

  // Assert that the transmission time is within the expected bounds
  EXPECT_TRUE(
    (seconds > lower_limit_sec || (seconds == lower_limit_sec && nanoseconds >= lower_limit_nsec)) &&
    (seconds < upper_limit_sec || (seconds == upper_limit_sec && nanoseconds <= upper_limit_nsec))
  ) << "Time taken to transmit the low bit is out of the boundaries of the error margin";
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
