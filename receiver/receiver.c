#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <hackrf.h>
#include <fftw3.h>
#include <signal.h>

#include "burger_stack_gui.h"
#include "secret_formula.h"

/*
    Key Params:
        - Sample Rate:              Number of samples taken per second (Hz)
        - Center Frequency:         Midpoint of the frequency range (span is spread around this)
        - FFT Size:                 Number of samples used for one FFT computation
        - Amount of Samples:        Number of FFT readings averaged to decode each bit
        - Transmitter Sample Rate:  Rate of which Transmitter is sending each bit (must match the transmitter)

   Worth Knowing:
        - Nyquist Rate: Minimum Sample Rate needed to accurately reconstruct a signal.
            (Usually atleast 2x the freq of the transmitter)
        - Symbol is 1 bit (for now)
*/

/*
====================
Configuration
====================
*/
#define SAMPLE_RATE 8000000
#define CENTER_FREQUENCY 63000000
#define FFT_SIZE 262144
#define NUM_OF_CONCATENATED_FFTS 2 // Number of FFTs averaged per symbol to reduce noise
#define TOTAL_FFT_SIZE (FFT_SIZE * NUM_OF_CONCATENATED_FFTS)
#define NUM_FFT_PER_SYMBOL 8
#define START_FREQ_RANGE 61500000.0 // Hz — must stay a double (don’t change to int)
#define END_FREQ_RANGE 61550000.0   // Hz — must stay a double (don’t change to int)
#define PACKET_STREAM_LENGTH 4 // Must match the transmitter packet size

// Thresholds to decide when a bit changes based on how much the signal goes up or down
#define INCREASE_THRESHOLD 15
#define DECREASE_THRESHOLD -15

/*
====================
Timing
====================
*/
#define TRANSMITTER_SAMPLE_RATE (0.5e6) // Transmitter sends 2 bit/second
#define PAUSE_US (TRANSMITTER_SAMPLE_RATE / NUM_FFT_PER_SYMBOL) // Time (micro seconds) between FFT reads to cover one transmitter bit


/*
====================
Frequency Calculations
====================
*/
#define BIN_SIZE (SAMPLE_RATE / FFT_SIZE)   // Frequency range each FFT slot covers (Hz)
#define BIN_OFFSET(i) ((double)(i) - FFT_SIZE / 2.0) // // How far this slot is from the center

/*
====================
Logging + Debug
====================
*/
#define LOG_FILE_NAME "fft_log.csv"
#define DEBUG_MODE 0

/*
====================
Calibration + Bitstream
====================
*/
#define CALIBRATION_STREAM_LENGTH 8

uint8_t CALIBRATION_STREAM[] = {1, 0, 1, 0, 1, 0, 1, 0}; // should be the same as the transmitter

// --- Frequency Span (Full FFT Range) ---
double start_freq_span = CENTER_FREQUENCY + BIN_OFFSET(0) * BIN_SIZE; // in Hz
double end_freq_span = CENTER_FREQUENCY + BIN_OFFSET(FFT_SIZE) * BIN_SIZE; // in Hz

// --- Sample Buffers ---
// TODO: Consider dynamic allocation if FFT_SIZE needs to be variable
static unsigned char sample_buffer[FFT_SIZE * NUM_OF_CONCATENATED_FFTS];
static unsigned char *first_sample = sample_buffer;
static unsigned char *second_sample = sample_buffer + (FFT_SIZE);

uint8_t first_sample_received = 0;

// --- Calibration State ---
int calibration_stream[CALIBRATION_STREAM_LENGTH];
uint8_t calibration_index = 0;
uint8_t calibrated = 0;

// --- Receiver State ---
static volatile int samples_already_collected = 0;

// --- FFT Buffers & Plan ---
fftw_complex *in;
fftw_complex *out;
fftw_plan plan; // Plan = config of FFT

// --- Logging ---
FILE *log_file; // Output file for magnitude + time logs
struct timespec log_time; // Timestamp for logs

// --- Bitstream Buffer (must match transmitter packet size) ---
uint8_t bit_stream[PACKET_STREAM_LENGTH];
uint8_t bit_stream_index = 0;

// Storing the read bits, for now its 100 bytes
char message_read[100];
uint8_t message_read_index = 0;

/*
    This function is called when HackRF gives us new data.

    - Each sample = 2 bytes (I and Q)
    - We want two chunks of FFT_SIZE samples to process
    - Since HackRF can’t give us both at once, we collect them one by one

    Make dynamic if needed.
*/
int rx_callback(hackrf_transfer *transfer)
{
    if (transfer->valid_length >= FFT_SIZE && !samples_already_collected) {
        if (first_sample_received) {
            memcpy(second_sample, transfer->buffer, FFT_SIZE);
            first_sample_received = 0;
            samples_already_collected = 1;
        } else {
            memcpy(first_sample, transfer->buffer, FFT_SIZE);
            first_sample_received = 1;
        }
    }

    return 0;
}

// Runs FFT and returns average signal strength between two frequency slots
double process_fft(uint32_t start_index, uint32_t end_index)
{
    // Load input buffer into FFT input as complex numbers
    for (int i = 0; i < TOTAL_FFT_SIZE; i++) {
        in[i][0] = (float)((int8_t)first_sample[2 * i]);
        in[i][1] = (float)((int8_t)first_sample[2 * i + 1]);
    }

    // Execute FFT using plan
    fftw_execute(plan);

    // Caluclate average magnitude
    double sum = 0;

    for (int i = start_index; i <= end_index; i++) {
        double real = out[i][0];
        double imag = out[i][1];
        double mag = sqrt((real * real) + (imag * imag));
        sum += mag;
    }

    uint32_t length = end_index - start_index;
    double average_mag = sum / length;

    return average_mag;
}

// Initializes and starts HackRF, returns 0 on success and -1 on error
int setup_hackrf(hackrf_device **device)
{
    if (hackrf_init() != HACKRF_SUCCESS) {
        fprintf(stderr, "HackRF init failed\n");
        return -1;
    }

    if (hackrf_open(device) != HACKRF_SUCCESS) {
        fprintf(stderr, "HackRF open failed\n");
        hackrf_exit();
        return -1;
    }

    hackrf_set_sample_rate(*device, SAMPLE_RATE);
    hackrf_set_freq(*device, CENTER_FREQUENCY);
    hackrf_set_amp_enable(*device, 1); // Enable internal amplifier

    if (hackrf_start_rx(*device, rx_callback, NULL) != HACKRF_SUCCESS) {
        fprintf(stderr, "HackRF RX start failed\n");
        hackrf_close(*device);
        hackrf_exit();
        return -1;
    }

    return 0;
}

void setup_fftw(void) {
    fftw_init_threads();
    fftw_plan_with_nthreads(4);

    in  = fftw_malloc(sizeof(fftw_complex) * TOTAL_FFT_SIZE);
    out = fftw_malloc(sizeof(fftw_complex) * TOTAL_FFT_SIZE);
    plan = fftw_plan_dft_1d(TOTAL_FFT_SIZE, in, out, FFTW_FORWARD, FFTW_MEASURE);
}

/*
    Compute the average signal magnitude in the given frequency range
    during the idle state.

    - Takes multiple FFT measurements over time to get an average
    - sample_length = 2 × NUM_FFT_PER_SYMBOL for better accuracy
*/
double compute_average_idle_magnitude(uint32_t start_index, uint32_t end_index)
{
    // Get the calculated frequencies
    double start_freq = CENTER_FREQUENCY + BIN_OFFSET(start_index) * BIN_SIZE;
    double end_freq = CENTER_FREQUENCY + BIN_OFFSET(end_index) * BIN_SIZE;

    // x2 for better accuracy
    uint32_t sample_length = 2 * NUM_FFT_PER_SYMBOL;
    uint32_t sample_index = 0;

    double idle_mag_sum = 0.0;

    while (sample_index < sample_length) {
        if (samples_already_collected) {
            double mag = process_fft(start_index, end_index);

            printf("[RX - Idle] Range: %.3f–%.3f MHz  Mag: %.2f\n",
                   start_freq / 1e6,
                   end_freq / 1e6,
                   mag);

            idle_mag_sum += mag;
            sample_index++;

            samples_already_collected = 0;
        }

        usleep(PAUSE_US);
    }

    return (idle_mag_sum / sample_length);
}

void cleanup_fftw(void)
{
    fftw_destroy_plan(plan);
    fftw_free(in);
    fftw_free(out);
}

void cleanup_hackrf(hackrf_device *device)
{
    hackrf_stop_rx(device);
    hackrf_close(device);
    hackrf_exit();
}

void handle_calibration(uint8_t current_bit)
{
    // Save received bit
    calibration_stream[calibration_index] = current_bit;

    if (DEBUG_MODE) {
        printf("[RX - Calibration] Validating bit: %d with received: %d\n", CALIBRATION_STREAM[calibration_index], current_bit);
    }

    if (CALIBRATION_STREAM[calibration_index] != current_bit) {
        if (DEBUG_MODE) {
            printf("[RX] Wrong bit detected, resetting...\n");
        }

        // wrong bit received, reset calibration stream. Keep current bit as latest
        calibration_stream[0] = current_bit;
        calibration_index = 1;
    }

    // If this was the last bit, set calibrated to 1
    if (calibration_index == (CALIBRATION_STREAM_LENGTH - 1)) {
        if (DEBUG_MODE) {
            printf("[RX - Calibration] DONE: ");

            for (int i = 0; i < CALIBRATION_STREAM_LENGTH; i++) {
                printf("%d", calibration_stream[i]);
            }

           printf("\n");
        }

        calibrated  = 1;
    }
}

void general_cleanup(hackrf_device *device)
{
    // Clean up FFTW
    cleanup_fftw();

    // Clean up HackRF
    if (device != NULL) {
        cleanup_hackrf(device);
    }

    // Clean up Burger Stack GUI
    cleanup_sdl();
}

// Handle Ctrl-C (SIGINT) to exit cleanly and print final message (for profiling/debug)
void handle_sigint(int sig)
{
    general_cleanup(NULL);

    message_read[message_read_index] = '\0';

    if (DEBUG_MODE) {
        printf("Message read: %s\n", message_read);
    }

    exit(EXIT_SUCCESS);
}

int main(void)
{
    // === Main Setup ===

    // Handle Ctrl-C (SIGINT) to exit cleanly and save final state (used for gprof)
    signal(SIGINT, handle_sigint);

    // Setup Burger Stack GUI
    if (setup_sdl() == -1) {
        fprintf(stderr, "Unable to setup SDL\n");
        exit(EXIT_FAILURE);
    }

    // Setup HackRF
    hackrf_device *device = NULL;
    if (setup_hackrf(&device) == -1) {
        fprintf(stderr, "setup_hackrf failed\n");
        cleanup_sdl();
        return EXIT_FAILURE;
    }

    // Save Logs for future processing
     log_file = fopen(LOG_FILE_NAME, "w");
     if (!log_file) {
         cleanup_sdl();
         cleanup_hackrf(device);
         perror("fopen(LOG_FILE_NAME)");
         exit(EXIT_FAILURE);
     }

    setup_fftw();

    // === Calculate IDLE Magnitude ===

    /*
        Calculate the FFT index range based on desired frequency range and bin size.
        Rounding errors are not really important for this use case.
    */
    uint32_t start_index = (START_FREQ_RANGE - start_freq_span) / BIN_SIZE;
    uint32_t end_index = (END_FREQ_RANGE - start_freq_span) / BIN_SIZE;

    printf("[RX] Calculating average idle mag...\n");

    double average_idle_mag = compute_average_idle_magnitude(start_index, end_index);

    printf("[RX] Average idle mag: %f\n", average_idle_mag);

    // === Decoding Setup ===

    // Start comparing with average magnitude of idle state
    double previous_mag = average_idle_mag;

    uint8_t current_bit = 0;

    uint8_t sample_length = NUM_FFT_PER_SYMBOL - 1; // the last sample is saved in a variable
    double sample_mags[sample_length];
    uint8_t sample_start_index = 0;
    uint8_t sample_end_index = sample_length;

    double sample_mag_sum = 0;

    // Track timing
    struct timespec ts;
    double mag, diff;

    // === Pairing ===
    /*
        Sync receiver to start slightly after the next whole second.
        Transmitter sends at exact second -> Receiver wakes at second + delay (e.g. 100ms).
    */
    struct timespec pairing_ts;
    double delay_ms = 100;
    double jitter_ns = 10e6; // Acceptable jitter window: first 10 ms of a second

    while (1) {
        clock_gettime(CLOCK_REALTIME, &pairing_ts); // Get current time

        // If within the first 10ms of a new second, schedule wakeup
        if (pairing_ts.tv_nsec < jitter_ns) {
            pairing_ts.tv_sec += 1; // Move to next second
            pairing_ts.tv_nsec = delay_ms * 1000000; // Add desired delay in ns
            clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &pairing_ts, NULL);
            break;
        }
    }

    // === Setup Receiver timing ===

    struct timespec target;

    // Synch with pairing
    target.tv_sec = pairing_ts.tv_sec;
    target.tv_nsec = pairing_ts.tv_nsec;

    const long pause_ns = (long)(PAUSE_US * 1000);

     // === Start Receiver Decoding ===

    while (1) {
        // Check if HackRF samples are ready
        if (!samples_already_collected) {
            continue;
        }

        // Wait until exact target time
        clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &target, NULL);

        // Get start time
        if (sample_start_index == 0) {
            clock_gettime(CLOCK_REALTIME, &ts);
        }

        mag = process_fft(start_index, end_index);

        // === Receiver Sampling ===
        int done_sampling = sample_start_index == sample_end_index;

        if (!done_sampling) {
            sample_mags[sample_start_index++] = mag;
            sample_mag_sum += mag;
        } else {
            // Done Sampling
            mag = (sample_mag_sum + mag) / NUM_FFT_PER_SYMBOL;

            // Save value to CSV
            if (DEBUG_MODE) {
                clock_gettime(CLOCK_REALTIME, &log_time);
                int64_t timestamp_ms = (int64_t)log_time.tv_sec * 1000 + log_time.tv_nsec / 1000000;

                fprintf(log_file, "%02ld:%02ld:%02ld.%03ld,%f\n",
                        (ts.tv_sec / 3600) % 24,
                        (ts.tv_sec / 60) % 60,
                        ts.tv_sec % 60,
                        ts.tv_nsec / 1000000,
                        mag);
            }

            /*
                Flip the bit only if there's a significant change in the expected direction:
                    - If current bit is 1, we expect a decrease to switch to 0
                    - If current bit is 0, we expect an increase to switch to 1
            */
            diff = (mag - previous_mag) / previous_mag * 100;

            current_bit = current_bit ? (diff <= DECREASE_THRESHOLD ? 0 : 1) :
                            (diff >= INCREASE_THRESHOLD ? 1 : 0);

            printf("[RX] %02ld:%02ld:%02ld.%03ld Mag: %.2f diff: %f BIT=%d\n",
                   (ts.tv_sec / 3600) % 24,
                   (ts.tv_sec / 60) % 60,
                   ts.tv_sec % 60,
                   ts.tv_nsec / 1000000,
                   mag,
                   diff,
                   current_bit);

            // === Calibration ===
            /*
                Bit decoding logic:

                - If not calibrated yet → add to calibration stream
                - When calibration stream reaches 8 bits (based on params) →
                    check against predefined calibration pattern
                - If it matches → start saving decoded bits
                - Bit rules (preferences):
                    - Idle is assumed to be 0 (low magnitude)
                    - Start bit should be 1 → signals the beginning of transmission
                    - If mismatch during calibration → reset stream but keep latest bit (sliding window)
                - Bits are stored big-endian:
                    e.g. char 'h' (01101000) → 0 is sent first and 1 second
            */

            if (!calibrated) {
                handle_calibration(current_bit);
            } else {
                // Save detected bit
                bit_stream[bit_stream_index++] = current_bit;

                if (bit_stream_index == PACKET_STREAM_LENGTH) {
                    // done saving (stored in big endian, check rules above)

                    // find out what the (4/packet stream length) number is
                    uint8_t number = 0;
                    uint8_t bit_stream_shift_amount = PACKET_STREAM_LENGTH - 1;

                    // Convert the received bit stream into a single byte (big-endian)
                    for (int i = 0; i < PACKET_STREAM_LENGTH; i++, bit_stream_shift_amount--) {
                        number |= (bit_stream[i] << bit_stream_shift_amount);
                    }

                    if (DEBUG_MODE) {
                        printf("[RX] Bit stream received: %d\n", number);
                    }

                    // Add image to screen
                    if (number == END_CHARACTER) {
                        break;
                    }

                    if (number >= secret_formula_lookup_length) {
                        printf("[RX] Unknown ingredient\n");
                        add_image(UNKNOWN);
                    } else {
                        if (DEBUG_MODE) {
                            printf("[RX] Ingredient: %s\n", secret_formula_lookup[number].image_path);
                        }
                        add_image(secret_formula_lookup[number].id);
                    }

                    message_read[message_read_index++] = number;
                    bit_stream_index = 0;
                }
            }
        }

        // Save and reset state for next set of samples
        previous_mag = mag;
        sample_start_index = 0;
        sample_mag_sum = 0;
        samples_already_collected = 0;

        // increase target time for next sample
        target.tv_nsec += pause_ns;

        // In case of overflow
        if (target.tv_nsec >= 1e9) {
            target.tv_sec += 1;
            target.tv_nsec -= 1e9;
        }
    }

    printf("[Rx] Done receiving!\n");

    while (1) {
        ; // to keep image displayed
    }

    general_cleanup(device);

    return EXIT_SUCCESS;
}
