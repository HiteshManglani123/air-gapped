#include <gtest/gtest.h>
#include <math.h>

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
};

TEST_F(TransmitterFixture, high_bit_transmission_time_within_expected_error_margin) {
  double error_margin = 0.1;
  
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

  printf("Set interval        : %ld.%ld sec\n", transmitter_get_interval(transmitter).tv_sec, transmitter_get_interval(transmitter).tv_nsec);
  printf("Lower limit         : %ld.%ld sec\n", seconds, (long)(transmitter_get_interval(transmitter).tv_nsec * (1-error_margin)));
  printf("Transmission time   : %ld.%ld sec\n", seconds, nanoseconds);
  printf("Upper limit         : %ld.%ld sec\n", seconds, (long)(transmitter_get_interval(transmitter).tv_nsec * (1+error_margin)));
  EXPECT_TRUE(
    (transmitter_get_interval(transmitter).tv_sec <= seconds) && (nanoseconds  < (long)(transmitter_get_interval(transmitter).tv_nsec*(1+error_margin))) &&
    (transmitter_get_interval(transmitter).tv_sec >= seconds) && (nanoseconds  > (long)(transmitter_get_interval(transmitter).tv_nsec*(1-error_margin)))
    );
}

TEST_F(TransmitterFixture, low_bit_transmission_time_within_expected_error_margin) {
  double error_margin = 0.1;
  
  struct timespec ts_start;
  struct timespec ts_end;
  clock_gettime(CLOCK_REALTIME, &ts_start);
  transmitter_send_bit(transmitter, LOW);
  clock_gettime(CLOCK_REALTIME, &ts_end);

  long seconds = ts_end.tv_sec - ts_start.tv_sec;
  long nanoseconds = ts_end.tv_nsec - ts_start.tv_nsec;

  if (nanoseconds < 0) {
      seconds--;
      nanoseconds += 1000000000; // 1 billion nanoseconds in a second
  }

  printf("Set interval        : %ld.%ld sec\n", transmitter_get_interval(transmitter).tv_sec, transmitter_get_interval(transmitter).tv_nsec);
  printf("Lower limit         : %ld.%ld sec\n", seconds, (long)(transmitter_get_interval(transmitter).tv_nsec * (1-error_margin)));
  printf("Transmission time   : %ld.%ld sec\n", seconds, nanoseconds);
  printf("Upper limit         : %ld.%ld sec\n", seconds, (long)(transmitter_get_interval(transmitter).tv_nsec * (1+error_margin)));
  EXPECT_TRUE(
    (transmitter_get_interval(transmitter).tv_sec <= seconds) && (nanoseconds  < (long)(transmitter_get_interval(transmitter).tv_nsec*(1+error_margin))) &&
    (transmitter_get_interval(transmitter).tv_sec >= seconds) && (nanoseconds  > (long)(transmitter_get_interval(transmitter).tv_nsec*(1-error_margin)))
    );
}

TEST_F(TransmitterFixture, calibration_starts_with_high)
{
  int length_even = 4;
  int length_odd = 5;
  int calibration_sequence_even[MAX_CALIBRATION_LENGTH];
  int calibration_sequence_odd[MAX_CALIBRATION_LENGTH];
  printf("Testing even length\n");
  transmitter_send_calibration(transmitter, length_even, calibration_sequence_even);
  printf("Testing odd length\n");
  transmitter_send_calibration(transmitter, length_odd, calibration_sequence_odd);

  printf("Even number: %d\n", calibration_sequence_even[0]);
  printf("Odd number : %d\n", calibration_sequence_odd[0]);

  EXPECT_TRUE(calibration_sequence_even[0] == 1 && calibration_sequence_odd[0] == 1);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
