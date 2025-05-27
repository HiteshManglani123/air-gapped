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

#define SAMPLE_RATE 8000000
#define FREQUENCY 63000000
// #define FFT_SIZE 65536 // 131072
#define FFT_SIZE 262144 // 262144

/*
// If we are transmitting 1 bt every second, to accurately receive and recreate the signal,
   we need to sample at least 2 times per second.
*/
#define AMOUNT_OF_SAMPLES 8

// Needs to be same as transmitter
#define TRANSMITTER_SAMPLE_RATE (0.5e6) // 1 every second

// Amount to pause after every sample
#define PAUSE (TRANSMITTER_SAMPLE_RATE / AMOUNT_OF_SAMPLES)

#define BIN_SIZE (SAMPLE_RATE / FFT_SIZE)
#define BIN_OFFSET(i) ((double)(i) - FFT_SIZE / 2.0)

/*
    The start and end frequency of the spectrum analyzer (waterfall)
*/
double start_freq_span = FREQUENCY + BIN_OFFSET(0) * BIN_SIZE; // in Hz
double end_freq_span = FREQUENCY + BIN_OFFSET(FFT_SIZE) * BIN_SIZE; // in Hz

// Start and end frequency to scan
double start_freq_range = 61500000; // in MHz
double end_freq_range = 61550000;   // in MHz

// double start_freq_range = 64500000; // in MHz
// double end_freq_range = 64550000;   // in MHz

// This will hold the hackrf values for every sample (2 bytes per element)
static unsigned char sample_buffer[FFT_SIZE * 2];
static unsigned char *sample_buffer2 = sample_buffer + (FFT_SIZE);

// Calibration
#define CALIBRATION_STREAM_LENGTH 8
uint8_t CALIBRATION_STREAM[] = {1, 0, 1, 0, 1, 0, 1, 0}; // should be the same as the transmitter

uint8_t calibration_index = 0;
int calibration_stream[CALIBRATION_STREAM_LENGTH];
uint8_t calibrated = 0;

uint8_t already_sampled = 0;

static volatile int ready = 0;

/*
    In and out buffer for fft.
    plan = config of fft
*/
fftw_complex *in;
fftw_complex *out;
fftw_plan plan;

// To save mag and time raw data
FILE *f;
struct timespec f_time;


// To store the incoming bits, needs to be same length as transmitter
#define BIT_STREAM_LENGTH 4
uint8_t bit_stream[BIT_STREAM_LENGTH];
uint8_t bit_stream_index = 0;

char message_read[100]; // for now 100 bytes idk
uint8_t message_read_index = 0;

int debug = 0;

// for gprof, exit gracefully on ctrl-c
void handle_sigint(int sig) {
    message_read[message_read_index] = '\0';
    cleanup_sdl();
    if (debug) {
        printf("Message read: %s\n", message_read);
    }
    exit(0);
}

int rx_callback(hackrf_transfer* transfer) {
    // this used to be FFT_SIZE * 2
    // this is dependant on 262k fft size btw, so there is a limit on transfer->valid_length which is 262k
    // if i want to store a 262k fft buffer, i need 262k * 2 buffer size, thats why i am collecting it 2x
    if (transfer->valid_length >= FFT_SIZE && !ready) {
        if (already_sampled) {
            memcpy(sample_buffer2, transfer->buffer, FFT_SIZE);
            // 2nd sample
            ready = 1;
            already_sampled = 0;
        } else {
            memcpy(sample_buffer, transfer->buffer, FFT_SIZE);
            already_sampled = 1;
        }

    }

    return 0;
}

// returns average magnitude of given start and end range (index)
double process_fft_and_print(uint32_t start_index, uint32_t end_index) {
    for (int i = 0; i < FFT_SIZE; i++) {
        in[i][0] = (float)((int8_t)sample_buffer[2 * i]);
        in[i][1] = (float)((int8_t)sample_buffer[2 * i + 1]);
    }

    fftw_execute(plan);
    uint32_t length = end_index - start_index;

    double sum = 0;
    for (int i = start_index; i <= end_index; i++) {
        double real = out[i][0];
        double imag = out[i][1];
        double mag = sqrt((real * real) + (imag * imag));
        sum += mag;
    }

    double average_mag = sum / length;

    return average_mag;
}

int setup_hackrf(hackrf_device *device) {
    if (hackrf_init() != HACKRF_SUCCESS) {
        fprintf(stderr, "HackRF init failed\n");
        return -1;
    }

    if (hackrf_open(&device) != HACKRF_SUCCESS) {
        fprintf(stderr, "HackRF open failed\n");
        hackrf_exit();
        return -1;
    }

    hackrf_set_sample_rate(device, SAMPLE_RATE);
    hackrf_set_freq(device, FREQUENCY);

    // Hackrf has a built in AMP
    hackrf_set_amp_enable(device, 1);

    if (hackrf_start_rx(device, rx_callback, NULL) != HACKRF_SUCCESS) {
        fprintf(stderr, "HackRF RX start failed\n");
        hackrf_close(device);
        hackrf_exit();
        return -1;
    }
}

void setup_fftw(void) {
    // Setup fft
    fftw_init_threads();
    fftw_plan_with_nthreads(4);

    // TODO: check if you need to do error checking here
    in  = fftw_malloc(sizeof(fftw_complex) * FFT_SIZE);
    out = fftw_malloc(sizeof(fftw_complex) * FFT_SIZE);
    plan = fftw_plan_dft_1d(FFT_SIZE, in, out, FFTW_FORWARD, FFTW_MEASURE);
}

double compute_average_magnitude(uint32_t start_index, uint32_t end_index) {
    // Get the calculated frequencies
    double start_freq = FREQUENCY + BIN_OFFSET(start_index) * BIN_SIZE;
    double end_freq = FREQUENCY + BIN_OFFSET(end_index) * BIN_SIZE;

    uint32_t sample_length = 2 * AMOUNT_OF_SAMPLES;
    uint32_t sample_index = 0;

    double idle_mag_sum = 0;

    while (sample_index < sample_length) {
        if (ready) {
            double mag = process_fft_and_print(start_index, end_index);
            printf("%.6f MHz - %.6f MHz: %2f\n", start_freq / 1e6, end_freq / 1e6, mag);
            idle_mag_sum += mag;
            sample_index++;
            ready = 0;
        }

        usleep(PAUSE);
    }

    return (idle_mag_sum / sample_length);
}

void cleanup_fftw(void) {
    fftw_destroy_plan(plan);
    fftw_free(in);
    fftw_free(out);
}

void cleanup_hackrf(hackrf_device *device) {
    hackrf_stop_rx(device);
    hackrf_close(device);
    hackrf_exit();
}

void handle_calibration(uint8_t current_bit) {
    // Save received bit
    calibration_stream[calibration_index] = current_bit;

    // if we are at the end
    if (calibration_index == CALIBRATION_STREAM_LENGTH) {
        // print calibration
        if (debug) {
            printf("Calibration: ");
            for (int i = 0; i < CALIBRATION_STREAM_LENGTH; i++) {
                printf("%d", calibration_stream[i]);
            }

           printf("\n");
        }

       calibrated = 1;
    } else {
        if (debug) {
            printf("[RX] CALIBRATION - Validating bit: %d with received: %d\n", CALIBRATION_STREAM[calibration_index], current_bit);
        }
        if (CALIBRATION_STREAM[calibration_index] != current_bit) {
            if (debug) {
                printf("Wrong bit, resetting...\n");
            }
            // wrong bit received, reset calibration stream. Keep current bit as latest
            calibration_stream[0] = current_bit;
            calibration_index = 0; // we increase it right after the loop to be 1, prob need to fix flow
        }
    }

    calibration_index++;
}

int main(void) {
    // exit gracefully on ctrl-c for gprof
    signal(SIGINT, handle_sigint);

    setup_sdl();

    // Save logs for future processing
     f = fopen("fft_log.csv", "w");

     if (!f) {
         fprintf(stderr, "Unable to open file!\n ");
         exit(1);
     }

    // Setup hackrf
    hackrf_device* device = NULL;

    if (setup_hackrf(device) == -1) {
        return EXIT_FAILURE;
    }

    setup_fftw();

    /*
    Calculate start and end index based on bin size
    Not too important if it rounds up/down
    */
    uint32_t start_index = (start_freq_range - start_freq_span) / BIN_SIZE;
    uint32_t end_index = (end_freq_range - start_freq_span) / BIN_SIZE;

    printf("Calculating average idle mag...\n");

    double average_idle_mag = compute_average_magnitude(start_index, end_index);

    printf("Average idle mag: %f\n", average_idle_mag);

    // Start comparing with average magnitude of idle state
    double previous_mag = average_idle_mag;

    uint8_t current_bit = 0;
    uint8_t current_bit_sum = 0;
    uint8_t bit_index = 0;

    uint8_t sample_length = AMOUNT_OF_SAMPLES - 1; // the last sample is saved in a variable

    double sample_mags[sample_length];
    double sample_mag_sum = 0;

    uint8_t sample_start_index = 0;
    uint8_t sample_end_index = sample_length;

    // to track timings
    struct timespec ts;

    double mag, diff;

    /*
        This pairing code tries to get the receiver to start at whole seconds + some delay.
        Transmitter sends at whole second -> Receiver receives at whole second + (e.g. 100ms)
    */
    struct timespec pairing_ts;

    double delay_ms = 100;

    // near start of a new second
    while (1) {
        clock_gettime(CLOCK_REALTIME, &pairing_ts);

        if (pairing_ts.tv_nsec < 10000000) {
            pairing_ts.tv_sec += 1;
            pairing_ts.tv_nsec = delay_ms * 1000000;
            clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &pairing_ts, NULL);
            break;
        }
    }

    struct timespec target;

    // Synch with pairing
    target.tv_sec = pairing_ts.tv_sec;
    target.tv_nsec = pairing_ts.tv_nsec;

    const long pause_ns = (long)(PAUSE * 1000);

    while (1) {
        // Wait until exact target time
        if (!ready) continue;
        clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &target, NULL);

        // Get current start time
        if (sample_start_index == 0) {
            clock_gettime(CLOCK_REALTIME, &ts);
        }

        mag = process_fft_and_print(start_index, end_index);
        diff = (mag - previous_mag) / previous_mag * 100;

        if (sample_start_index == sample_end_index) {
            uint8_t increase_treshhold = 15;
            int8_t decrease_treshhold = -15; // easier to detect decrease

            mag = (sample_mag_sum + mag) / AMOUNT_OF_SAMPLES;
            diff = (mag - previous_mag) / previous_mag * 100;

            // save to csv
            // clock_gettime(CLOCK_REALTIME, &f_time);
            // int64_t timestamp_ms = (int64_t)f_time.tv_sec * 1000 + f_time.tv_nsec / 1000000;

            if (debug) {
                fprintf(f, "%02ld:%02ld:%02ld.%03ld,%f\n",
                        (ts.tv_sec / 3600) % 24,
                        (ts.tv_sec / 60) % 60,
                        ts.tv_sec % 60,
                        ts.tv_nsec / 1000000,
                        mag);
            }

            // if current bit is 1, we only care if its decreasing and vice versa for current bit is equal to 0
            current_bit = current_bit ? (diff <= decrease_treshhold ? 0 : 1) :
                            (diff >= increase_treshhold ? 1 : 0);


            printf("[RX] %02ld:%02ld:%02ld.%03ld Mag: %.2f diff: %f BIT=%d\n",
                   (ts.tv_sec / 3600) % 24,
                   (ts.tv_sec / 60) % 60,
                   ts.tv_sec % 60,
                   ts.tv_nsec / 1000000,
                   mag,
                   diff,
                   current_bit);

            /*
                if not calibrated yet -> add to calibration stream
                once calibrated index length = 8 = done
                check if calibrated mateched the pre-defined calibrations stream

                if calibrated -> save the bit

                rules:
                - start bit is 1 (ideally the magnitude of the receiver reading the idle state in the beginning)
                - if wrong, reset calibration stream but keep latest read
                - sliding window vibes

                for not the bit is stored in big endian:
                e.g char h in binary is 01101000 -> 0 would be sent first
            */

            // If not calibrated yet
            if (!calibrated) {
                handle_calibration(current_bit);
            }

            // shitty design pattern tbh, need to fix
            if (calibrated) {
                // Already calibrated

                // save bit
                bit_stream[bit_stream_index++] = current_bit;

                if (bit_stream_index == BIT_STREAM_LENGTH) {
                    // done saving (stored in big endian, check rules above)

                    // find out what the (4/bit stream length) number is
                    uint8_t number = 0;
                    uint8_t bit_stream_shift_amount = BIT_STREAM_LENGTH - 1;

                    for (int i = 0; i < BIT_STREAM_LENGTH; i++, bit_stream_shift_amount--) {
                        number |= (bit_stream[i] << bit_stream_shift_amount);
                    }

                    if (debug) {
                        printf("Bit stream: %d\n", number);
                    }

                    if (number == END_CHARACTER) {
                        break;
                    }

                    if (number >= secret_formula_lookup_length) {
                        printf("[RX] Unknown ingredient\n");
                        add_image(UNKNOWN);
                    }else {
                        if (debug) {
                            printf("Getting ingredients\n");
                            printf("Ingredient: %s\n", secret_formula_lookup[number].image_path);
                        }
                        add_image(secret_formula_lookup[number].id);
                    }

                    message_read[message_read_index++] = number;
                    bit_stream_index = 0;
                }
            }

            previous_mag = mag;
            sample_start_index = 0;
            sample_mag_sum = 0;
        } else {
            sample_mags[sample_start_index++] = mag;
            sample_mag_sum += mag;
        }

        ready = 0;

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

    // Clean up FFTW
    cleanup_fftw();

    // Clean up HackRF
    cleanup_hackrf(device);

    cleanup_sdl();
    return EXIT_SUCCESS;
}
