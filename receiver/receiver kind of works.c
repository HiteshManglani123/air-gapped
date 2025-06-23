#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <hackrf.h>
#include "kiss_fft.h"
#include <signal.h>

#define SAMPLE_RATE 8000000
#define FREQUENCY 63000000
#define FFT_SIZE 131072

#define PAUSE (0.5e6) // 1 second = 1e6 micro seconds

#define BIN_SIZE (SAMPLE_RATE / FFT_SIZE)
#define BIN_OFFSET(i) ((double)(i) - FFT_SIZE / 2.0)


double start_freq_span = FREQUENCY + BIN_OFFSET(0) * BIN_SIZE; // in Hz
double end_freq_span = FREQUENCY + BIN_OFFSET(FFT_SIZE) * BIN_SIZE; // in Hz

// Start and end frequency to scan
double start_freq_range = 63190000; // in MHz
double end_freq_range = 63210000;   // in MHz

static unsigned char sample_buffer[FFT_SIZE * 2];
static volatile int ready = 0;

//
kiss_fft_cpx in[FFT_SIZE], out[FFT_SIZE];

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
double process_fft_and_print(kiss_fft_cfg config, uint32_t start_index, uint32_t end_index) {
    for (int i = 0; i < FFT_SIZE; i++) {
        in[i].r = (float)((int8_t)sample_buffer[2 * i]);
        in[i].i = (float)((int8_t)sample_buffer[2 * i + 1]);
    }

    kiss_fft(config, in, out);

    uint32_t length = end_index - start_index;

    double sum = 0;

    for (int i = start_index; i <= end_index; i++) {
        double mag = sqrt(out[i].r * out[i].r + out[i].i * out[i].i);
        sum += mag;
    }

    double average_mag = sum / length;

    return average_mag;
}

int main() {
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

    // get soem sort of average to compare to
    double average_sum = 0;

    uint32_t pairing_length = 5;
    uint32_t pairing_index = 0;

    // create config for fft
    kiss_fft_cfg config = kiss_fft_alloc(FFT_SIZE, 0, NULL, NULL);

    printf("Calculating average idle mag...\n");

    // todo: fix this trash loop lol
    while (1 && pairing_index < pairing_length) {
        if (ready) {
            double mag = process_fft_and_print(config, start_index, end_index);
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

    double first_sample_mag;
    double first_sample_diff;

    uint8_t already_sampled = 0;


    while (1) {
        if (ready) {
            double mag = process_fft_and_print(config, start_index, end_index);

            double diff = (mag - previous_mag) / previous_mag * 100;
            //printf("Diff: %f\n", diff);

            if (alread_sampled) {
                // logic to check what it actually is

                continue;
            }

            previous_mag = mag;

            /*
                if diff is signigicant, bit flip
                if it's a 1, i only care if it decreases by a certain treshhold
                if it's a 0, i only care if it increased by a certain threshold
            */

            // diff in percentage
            uint8_t increase_treshhold = 50;
            int8_t decrease_treshhold = -50;

            if (current_bit == 1) {
                // if it's below a certain treshhold, flip bit
                current_bit = (diff <= decrease_treshhold ? 0 : 1);
            } else {
                // if it's above a certain treshhold, flip bit
                current_bit = (diff >= increase_treshhold ? 1 : 0);
            }


            // calc percentage to see if 1 or 0 (for now 400%)
            // current_bit = (diff >= tresh_hold ? !current_bit : current_bit);
            // uint8_t bit = (mag >= (average_idle_mag * tresh_hold)) ? 1 : 0;

            ready = 0;
            if (bit_index == 1) {
                struct timespec ts;
                clock_gettime(CLOCK_REALTIME, &ts);

                if (current_bit_sum == 0) {
                    current_bit = 0;
                } else if (current_bit_sum == 2) {
                    current_bit = 1;
                }
                // leave current_bit as is if it's 1

                printf("[RX] %02ld:%02ld:%02ld.%03ld Mag: %.2f diff: %f BIT=%d\n",
                       (ts.tv_sec / 3600) % 24,
                       (ts.tv_sec / 60) % 60,
                       ts.tv_sec % 60,
                       ts.tv_nsec / 1000000,
                       mag,
                       diff,
                       current_bit);

                current_bit_sum = 0;
                bit_index = 0;
            } else {
                bit_index++;
                current_bit_sum += current_bit;
            }


            // printf("%.6f MHz - %.6f MHz: %2f - bit: %d\n", start_freq / 1e6, end_freq / 1e6, mag, bit);

            usleep(PAUSE);
        }
    }

    free(config);

    hackrf_stop_rx(device);
    hackrf_close(device);
    hackrf_exit();
    return EXIT_SUCCESS;
}
