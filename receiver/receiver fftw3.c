#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <hackrf.h>
// #include "kiss_fft.h"
#include <fftw3.h>
#include <signal.h>


#define SAMPLE_RATE 8000000
#define FREQUENCY 63000000
// #define FFT_SIZE 131072
#define FFT_SIZE 131072

/*
i am having these problems:
- the timings are not accurate enough
- i could have a sampling rate (PAUSE) issue where i need to increase it to around 4x.

so i take 4x samples while the tx is transmitting 1 sample.
*/

#define AMOUNT_OF_SAMPLES 4
#define TRANSMITTER_SAMPLE_RATE 1e6 // 1 every second
#define PAUSE (TRANSMITTER_SAMPLE_RATE / AMOUNT_OF_SAMPLES) // 1 second = 1e6 micro seconds

#define BIN_SIZE (SAMPLE_RATE / FFT_SIZE)
#define BIN_OFFSET(i) ((double)(i) - FFT_SIZE / 2.0)


double start_freq_span = FREQUENCY + BIN_OFFSET(0) * BIN_SIZE; // in Hz
double end_freq_span = FREQUENCY + BIN_OFFSET(FFT_SIZE) * BIN_SIZE; // in Hz

// Start and end frequency to scan
double start_freq_range = 63400000; // in MHz
double end_freq_range = 63410000;   // in MHz
// double start_freq_range = 63190000; // in MHz
// double end_freq_range = 63210000;   // in MHz

static unsigned char sample_buffer[FFT_SIZE * 2];
static volatile int ready = 0;

//
// kiss_fft_cpx in[FFT_SIZE], out[FFT_SIZE];


// to check where the bottle necks are
struct timespec start,end;

fftw_complex *in;
fftw_complex *out;
fftw_plan plan;

// for gprof, exit gracefully on ctrl-c
void handle_sigint(int sig) {
    exit(0);
}

int rx_callback(hackrf_transfer* transfer) {
    if (transfer->valid_length >= FFT_SIZE * 2 && !ready) {
        memcpy(sample_buffer, transfer->buffer, FFT_SIZE * 2);
        ready = 1;
    }

    return 0;
}

// returns average magnitude of given start and end range (index)
double process_fft_and_print(uint32_t start_index, uint32_t end_index) {
    clock_gettime(CLOCK_MONOTONIC, &start);

    // for (int i = 0; i < FFT_SIZE; i++) {
    //     in[i].r = (float)((int8_t)sample_buffer[2 * i]);
    //     in[i].i = (float)((int8_t)sample_buffer[2 * i + 1]);
    // }

    for (int i = 0; i < FFT_SIZE; i++) {
        in[i][0] = (float)((int8_t)sample_buffer[2 * i]);
        in[i][1] = (float)((int8_t)sample_buffer[2 * i + 1]);
    }


    clock_gettime(CLOCK_MONOTONIC, &end);

    double duration_ns = (end.tv_sec - start.tv_sec) * 1000000 +
                        (end.tv_nsec - start.tv_nsec) / 1000.0;

    printf("Loop 1 diff: %f\n", duration_ns);

    clock_gettime(CLOCK_MONOTONIC, &start);

    fftw_execute(plan);
    // kiss_fft(config, in, out);

    clock_gettime(CLOCK_MONOTONIC, &end);

    duration_ns = (end.tv_sec - start.tv_sec) * 1000000 +
                        (end.tv_nsec - start.tv_nsec) / 1000.0;

    printf("FFT diff: %f\n", duration_ns);

    uint32_t length = end_index - start_index;

    double sum = 0;

    clock_gettime(CLOCK_MONOTONIC, &start);

    // for (int i = start_index; i <= end_index; i++) {
    //     double mag = sqrt(out[i].r * out[i].r + out[i].i * out[i].i);
    //     sum += mag;
    // }
    for (int i = start_index; i <= end_index; i++) {
        double real = out[i][0];
        double imag = out[i][1];
        double mag = sqrt(real * real + imag * imag);
        sum += mag;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    duration_ns = (end.tv_sec - start.tv_sec) * 1000000 +
                (end.tv_nsec - start.tv_nsec) / 1000.0;


    double average_mag = sum / length;

    printf("Loop2 diff: %f\n", duration_ns);

    return average_mag;
}

int main(void) {
    fftw_init_threads();
    fftw_plan_with_nthreads(4);

    in  = fftw_malloc(sizeof(fftw_complex) * FFT_SIZE);
    out = fftw_malloc(sizeof(fftw_complex) * FFT_SIZE);
    plan = fftw_plan_dft_1d(FFT_SIZE, in, out, FFTW_FORWARD, FFTW_MEASURE);

    // exit gracefully on cntrl-c for gprof
    signal(SIGINT, handle_sigint);

    hackrf_device* device = NULL;

    if (hackrf_init() != HACKRF_SUCCESS) {
        fprintf(stderr, "HackRF init failed\n");
        return EXIT_FAILURE;
    }

    if (hackrf_open(&device) != HACKRF_SUCCESS) {
        fprintf(stderr, "HackRF open failed\n");
        hackrf_exit();
        return EXIT_FAILURE;
    }

    hackrf_set_sample_rate(device, SAMPLE_RATE);
    hackrf_set_freq(device, FREQUENCY);

    // Hackrf has a built in AMP
    hackrf_set_amp_enable(device, 1);

    if (hackrf_start_rx(device, rx_callback, NULL) != HACKRF_SUCCESS) {
        fprintf(stderr, "HackRF RX start failed\n");
        hackrf_close(device);
        hackrf_exit();
        return EXIT_FAILURE;
    }

    /*
    Calculate start and end index based on bin size
    Not too important if it rounds up/down
    */
    uint32_t start_index = (start_freq_range - start_freq_span) / BIN_SIZE;
    uint32_t end_index = (end_freq_range - start_freq_span) / BIN_SIZE;

    double start_freq = FREQUENCY + BIN_OFFSET(start_index) * BIN_SIZE;
    double end_freq = FREQUENCY + BIN_OFFSET(end_index) * BIN_SIZE;

    // get some sort of average to compare to
    double average_sum = 0;

    uint32_t pairing_length = 1 * AMOUNT_OF_SAMPLES;
    uint32_t pairing_index = 0;

    // create config for fft
    // kiss_fft_cfg config = kiss_fft_alloc(FFT_SIZE, 0, NULL, NULL);

    printf("Calculating average idle mag...\n");



    while (pairing_index < pairing_length) {
        if (ready) {
            double mag = process_fft_and_print(start_index, end_index);
            printf("%.6f MHz - %.6f MHz: %2f\n", start_freq / 1e6, end_freq / 1e6, mag);
            average_sum += mag;
            pairing_index++;
            ready = 0;
        }
        usleep(PAUSE);
    }

    double average_idle_mag = average_sum / pairing_length;

    printf("Average idle mag: %f\n", average_idle_mag);

    double previous_mag = average_idle_mag;

    uint8_t current_bit = 0;
    uint8_t current_bit_sum = 0;
    uint8_t bit_index = 0;


    uint8_t sample_length = AMOUNT_OF_SAMPLES - 1; // the last sample is saved in a variable

    double sample_mags[sample_length];
    // double sample_diffs[sample_length];
    // double sample_bits[sample_length];

    uint8_t sample_start_index = 0;
    uint8_t sample_end_index = sample_length;

    struct timespec prev_ts;
    struct timespec ts;
    uint8_t has_prev_ts = 0;

    double mag;
    double diff;

    while (1) {
        if (ready) {
            mag = process_fft_and_print(start_index, end_index);

            // difference in percentage
            diff = (mag - previous_mag) / previous_mag * 100;

            /*
                if diff is signigicant, bit flip
                if it's a 1, i only care if it decreases by a certain treshhold
                if it's a 0, i only care if it increased by a certain threshold
            */

            if (sample_start_index == sample_end_index) {
                // this is the last sample
                uint8_t increase_treshhold = 30;
                int8_t decrease_treshhold = -30;

                mag = ((sample_mags[0] + sample_mags[1]) + (sample_mags[2] + mag)) / AMOUNT_OF_SAMPLES;

                diff = (mag - previous_mag) / previous_mag * 100;

                if (current_bit == 1) {
                    // if it's below a certain treshhold, flip bit
                    current_bit = (diff <= decrease_treshhold ? 0 : 1);
                } else {
                    // if it's above a certain treshhold, flip bit
                    current_bit = (diff >= increase_treshhold ? 1 : 0);
                }

                clock_gettime(CLOCK_REALTIME, &ts);

                printf("[RX] %02ld:%02ld:%02ld.%03ld Mag: %.2f diff: %f BIT=%d\n",
                       (ts.tv_sec / 3600) % 24,
                       (ts.tv_sec / 60) % 60,
                       ts.tv_sec % 60,
                       ts.tv_nsec / 1000000,
                       mag,
                       diff,
                       current_bit);

                if (has_prev_ts) {
                    double diff_microseconds = (ts.tv_sec - prev_ts.tv_sec) * 1000000.0
                                              + (ts.tv_nsec - prev_ts.tv_nsec) / 1000.0;

                    printf("Diff in time: %.5f\n", diff_microseconds);
                }

                prev_ts = ts;
                has_prev_ts = 1;

                // update previous mag based on 2nd sample
                previous_mag = mag;
                sample_start_index = 0;

                ready = 0;
                usleep(PAUSE);
                continue;
            }

            // this is the first sample so save the values
            sample_mags[sample_start_index] = mag;
            // first_sample_diff[sample_index] = diff;
            // first_sample_bit[sample_index] = current_bit;

            // toggle alread_sampled
            ready = 0;
            sample_start_index++;
            usleep(PAUSE);
        }
    }

    fftw_destroy_plan(plan);
    fftw_free(in);
    fftw_free(out);
    // free(config);

    hackrf_stop_rx(device);
    hackrf_close(device);
    hackrf_exit();
    return EXIT_SUCCESS;
}
