#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include <fftw3.h>

// Include the real hackrf.h to get proper types and constants
#include <hackrf.h>

// Define this to prevent the main function in receiver.c from being compiled
#define main receiver_main

// Mock hardware functions with correct signatures
int hackrf_init(void) { return HACKRF_SUCCESS; }
int hackrf_open(hackrf_device **device) { return HACKRF_SUCCESS; }
int hackrf_set_sample_rate(hackrf_device *device, const double sample_rate) { return HACKRF_SUCCESS; }
int hackrf_set_freq(hackrf_device *device, const uint64_t freq_hz) { return HACKRF_SUCCESS; }
int hackrf_set_amp_enable(hackrf_device *device, const uint8_t value) { return HACKRF_SUCCESS; }
int hackrf_start_rx(hackrf_device *device, int (*callback)(hackrf_transfer *), void *ctx) { return HACKRF_SUCCESS; }
int hackrf_stop_rx(hackrf_device *device) { return HACKRF_SUCCESS; }
int hackrf_close(hackrf_device *device) { return HACKRF_SUCCESS; }
int hackrf_exit(void) { return HACKRF_SUCCESS; }

// Mock GUI functions
int setup_sdl(void) { return 0; }
void cleanup_sdl(void) {}
void add_image(int id) {}

// Include receiver functions
#include "../receiver.c"

// Undefine the macro so we can define our own main
#undef main

// Test counters
int tests_passed = 0;
int tests_failed = 0;

// Test pairing timing logic
int test_pairing_timing(void) {
    printf("Testing Pairing Timing...\n");
    int module_passed = 0;
    int module_total = 0;
    
    struct timespec pairing_ts;
    struct timespec before_setup;
    
    clock_gettime(CLOCK_REALTIME, &before_setup);
    setup_pairing(&pairing_ts);
    
    // After setup_pairing returns, we should be at or past the target time
    // The function sleeps until ~100ms after the next whole second
    struct timespec after_setup;
    clock_gettime(CLOCK_REALTIME, &after_setup);
    
    // Check if we're now at or past the target time
    if (after_setup.tv_sec >= pairing_ts.tv_sec && 
        (after_setup.tv_sec > pairing_ts.tv_sec || after_setup.tv_nsec >= pairing_ts.tv_nsec)) {
        module_passed++;
        printf("  ✓ Pairing timing correct\n");
    } else {
        printf("  ✗ Pairing timing failed\n");
    }
    module_total++;
    
    printf("  Pairing Timing: %d/%d tests passed\n\n", module_passed, module_total);
    tests_passed += module_passed;
    tests_failed += (module_total - module_passed);
    return module_passed == module_total;
}

// Test calibration logic
int test_calibration(void) {
    printf("Testing Calibration Module...\n");
    int module_passed = 0;
    int module_total = 0;
    
    // Test 1: Happy path - all 8 bits correct
    calibration_index = 0;
    calibrated = 0;
    for (int i = 0; i < CALIBRATION_STREAM_LENGTH; i++) {
        handle_calibration(CALIBRATION_STREAM[i]);
    }
    
    if (calibrated == 1) {
        module_passed++;
        printf("  ✓ Happy path\n");
    } else {
        printf("  ✗ Happy path failed\n");
    }
    module_total++;
    
    // Test 2: All wrong bits
    calibration_index = 0;
    calibrated = 0;
    for (int i = 0; i < CALIBRATION_STREAM_LENGTH; i++) {
        handle_calibration(0);  // Always send 0, which is wrong for odd positions
    }
    
    if (calibrated == 0) {
        module_passed++;
        printf("  ✓ All wrong bits\n");
    } else {
        printf("  ✗ All wrong bits failed\n");
    }
    module_total++;
    
    // Test 3: Early mistake then correct
    calibration_index = 0;
    calibrated = 0;
    handle_calibration(!CALIBRATION_STREAM[0]); // Wrong first bit
    for (int i = 0; i < CALIBRATION_STREAM_LENGTH; i++) {
        handle_calibration(CALIBRATION_STREAM[i]);
    }
    
    if (calibrated == 1) {
        module_passed++;
        printf("  ✓ Early mistake recovery\n");
    } else {
        printf("  ✗ Early mistake recovery failed\n");
    }
    module_total++;
    
    // Test 4: Middle mistake
    calibration_index = 0;
    calibrated = 0;
    for (int i = 0; i < 4; i++) {
        handle_calibration(CALIBRATION_STREAM[i]);
    }
    handle_calibration(!CALIBRATION_STREAM[4]); // Wrong middle bit
    for (int i = 4; i < CALIBRATION_STREAM_LENGTH; i++) {
        handle_calibration(CALIBRATION_STREAM[i]);
    }
    
    if (calibrated == 0) {
        module_passed++;
        printf("  ✓ Middle mistake handling\n");
    } else {
        printf("  ✗ Middle mistake handling failed\n");
    }
    module_total++;
    
    printf("  Calibration: %d/%d tests passed\n\n", module_passed, module_total);
    tests_passed += module_passed;
    tests_failed += (module_total - module_passed);
    return module_passed == module_total;
}

// Test bit decoding logic
int test_bit_decoding(void) {
    printf("Testing Bit Decoding Module...\n");
    int module_passed = 0;
    int module_total = 0;
    
    // Test 1: 0 to 1 transition (happy path)
    uint8_t current_bit = 0;
    double diff = 20.0;  // Above INCREASE_THRESHOLD (15)
    uint8_t new_bit = decode_bit(current_bit, diff);
    
    if (new_bit == 1) {
        module_passed++;
        printf("  ✓ 0 to 1 transition\n");
    } else {
        printf("  ✗ 0 to 1 transition failed\n");
    }
    module_total++;
    
    // Test 2: 1 to 0 transition (happy path)
    current_bit = 1;
    diff = -20.0;  // Below DECREASE_THRESHOLD (-15)
    new_bit = decode_bit(current_bit, diff);
    
    if (new_bit == 0) {
        module_passed++;
        printf("  ✓ 1 to 0 transition\n");
    } else {
        printf("  ✗ 1 to 0 transition failed\n");
    }
    module_total++;
    
    // Test 3: Edge case - exact threshold
    current_bit = 0;
    diff = 15.0;  // Exactly INCREASE_THRESHOLD
    new_bit = decode_bit(current_bit, diff);
    
    if (new_bit == 1) {
        module_passed++;
        printf("  ✓ Exact threshold\n");
    } else {
        printf("  ✗ Exact threshold failed\n");
    }
    module_total++;
    
    printf("  Bit Decoding: %d/%d tests passed\n\n", module_passed, module_total);
    tests_passed += module_passed;
    tests_failed += (module_total - module_passed);
    return module_passed == module_total;
}

// Test packet reconstruction logic
int test_packet_reconstruction(void) {
    printf("Testing Packet Reconstruction Module...\n");
    int module_passed = 0;
    int module_total = 0;
    
    // Test 1: Basic big-endian conversion
    uint8_t bit_stream[] = {1, 0, 1, 0};  // Should become 10 (1010 binary)
    uint8_t result = reconstruct_packet(bit_stream, 4);
    
    if (result == 10) {
        module_passed++;
        printf("  ✓ Basic big-endian conversion\n");
    } else {
        printf("  ✗ Basic big-endian conversion failed\n");
    }
    module_total++;
    
    // Test 2: All zeros
    uint8_t zeros[] = {0, 0, 0, 0};
    result = reconstruct_packet(zeros, 4);
    
    if (result == 0) {
        module_passed++;
        printf("  ✓ All zeros\n");
    } else {
        printf("  ✗ All zeros failed\n");
    }
    module_total++;
    
    // Test 3: All ones
    uint8_t ones[] = {1, 1, 1, 1};
    result = reconstruct_packet(ones, 4);
    
    if (result == 15) {
        module_passed++;
        printf("  ✓ All ones\n");
    } else {
        printf("  ✗ All ones failed\n");
    }
    module_total++;
    
    printf("  Packet Reconstruction: %d/%d tests passed\n\n", module_passed, module_total);
    tests_passed += module_passed;
    tests_failed += (module_total - module_passed);
    return module_passed == module_total;
}

// Test runner
void run_tests(void) {
    printf("=== Receiver Test Suite ===\n\n");
    
    test_pairing_timing();
    test_calibration();
    test_bit_decoding();
    test_packet_reconstruction();
    
    printf("=== Final Results ===\n");
    printf("Total: %d/%d tests passed\n", tests_passed, tests_passed + tests_failed);
    
    if (tests_failed == 0) {
        printf("All tests passed!\n");
    } else {
        printf("%d tests failed\n", tests_failed);
    }
}

// Our test main function
int main(void) {
    run_tests();
    return (tests_failed == 0) ? 0 : 1;
} 