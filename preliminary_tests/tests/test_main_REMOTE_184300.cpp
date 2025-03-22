#include <gtest/gtest.h>
#include <cmath>
#include <iostream>

extern "C" {
    #include <transmitter.h>
}

// Helper function to convert seconds to timespec
extern "C" void _seconds_to_timespec(double seconds, struct timespec *ts) {
    ts->tv_sec = static_cast<time_t>(std::floor(seconds));
    ts->tv_nsec = static_cast<long>((seconds - ts->tv_sec) * 1e9);
}

// Fixture for Transmitter tests
class TransmitterFixture : public testing::Test {
public:
    TransmitterFixture() {
        double interval_in_secs = 0.123;
        transmitter = transmitter_create(interval_in_secs);
    }

    ~TransmitterFixture() override {
        // Clean up resources if necessary
    }

    void SetUp() override {}
    void TearDown() override {}

    struct Transmitter *transmitter;
};

// Helper function to calculate elapsed time in seconds and nanoseconds
void calculate_elapsed_time(const struct timespec &start, const struct timespec &end, long &seconds, long &nanoseconds) {
    seconds = end.tv_sec - start.tv_sec;
    nanoseconds = end.tv_nsec - start.tv_nsec;

    if (nanoseconds < 0) {
        seconds--;
        nanoseconds += 1000000000; // 1 billion nanoseconds in a second
    }
}

// Helper function to log timing details
void log_timing_details(const struct timespec &interval, long seconds, long nanoseconds, double error_margin) {
    auto format_time = [](long sec, long nsec) {
        std::ostringstream oss;
        oss << sec << "." << std::setfill('0') << std::setw(9) << nsec;
        return oss.str();
    };

    std::cout << "Set interval        : " << format_time(interval.tv_sec, interval.tv_nsec) << " sec\n";
    std::cout << "Lower limit         : " << format_time(interval.tv_sec, static_cast<long>(interval.tv_nsec * (1 - error_margin))) << " sec\n";
    std::cout << "Transmission time   : " << format_time(seconds, nanoseconds) << " sec\n";
    std::cout << "Upper limit         : " << format_time(interval.tv_sec, static_cast<long>(interval.tv_nsec * (1 + error_margin))) << " sec\n";
}

// Test high bit transmission timing
TEST_F(TransmitterFixture, HighBitTransmissionTimeWithinExpectedErrorMargin) {
    double error_margin = 0.4;

    struct timespec ts_start, ts_end;
    clock_gettime(CLOCK_REALTIME, &ts_start);
    transmitter_send_bit(transmitter, HIGH);
    clock_gettime(CLOCK_REALTIME, &ts_end);

    long seconds, nanoseconds;
    calculate_elapsed_time(ts_start, ts_end, seconds, nanoseconds);

    struct timespec interval = transmitter_get_interval(transmitter);
    log_timing_details(interval, seconds, nanoseconds, error_margin);

    EXPECT_GE(seconds * 1e9 + nanoseconds, interval.tv_sec * 1e9 + interval.tv_nsec * (1 - error_margin));
    EXPECT_LE(seconds * 1e9 + nanoseconds, interval.tv_sec * 1e9 + interval.tv_nsec * (1 + error_margin));
}

// Test low bit transmission timing
TEST_F(TransmitterFixture, LowBitTransmissionTimeWithinExpectedErrorMargin) {
    double error_margin = 0.1;

    struct timespec ts_start, ts_end;
    clock_gettime(CLOCK_REALTIME, &ts_start);
    transmitter_send_bit(transmitter, LOW);
    clock_gettime(CLOCK_REALTIME, &ts_end);

    long seconds, nanoseconds;
    calculate_elapsed_time(ts_start, ts_end, seconds, nanoseconds);

    struct timespec interval = transmitter_get_interval(transmitter);
    log_timing_details(interval, seconds, nanoseconds, error_margin);

    EXPECT_GE(seconds * 1e9 + nanoseconds, interval.tv_sec * 1e9 + interval.tv_nsec * (1 - error_margin));
    EXPECT_LE(seconds * 1e9 + nanoseconds, interval.tv_sec * 1e9 + interval.tv_nsec * (1 + error_margin));
}

// Test calibration sequence starts with HIGH
TEST_F(TransmitterFixture, CalibrationStartsWithHigh) {
    int length_even = 4;
    int length_odd = 5;
    
    int calibration_sequence_even[MAX_CALIBRATION_LENGTH] = {0};
    int calibration_sequence_odd[MAX_CALIBRATION_LENGTH] = {0};

    std::cout << "Testing even length\n";
    transmitter_send_calibration(transmitter, length_even, calibration_sequence_even);

    std::cout << "Testing odd length\n";
    transmitter_send_calibration(transmitter, length_odd, calibration_sequence_odd);

    std::cout << "Even number: " << calibration_sequence_even[0] << "\n";
    std::cout << "Odd number : " << calibration_sequence_odd[0] << "\n";

    EXPECT_EQ(calibration_sequence_even[0], HIGH);
    EXPECT_EQ(calibration_sequence_odd[0], HIGH);
}

// Main function for running tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

