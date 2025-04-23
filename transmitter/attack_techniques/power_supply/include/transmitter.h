#ifndef TRANSMITTER_H
#define TRANSMITTER_H

#include <pthread.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>

#define HIGH 1
#define LOW 0
#define MAX_CALIBRATION_LENGTH 8

#define HZ 1e0
#define KHZ 1e3
#define MHZ 1e6

/**
 * @example
 *
 * // Set frequency to 1 mHz
 * FREQUENCY(1, MHZ)
 * // or
 * FREQUENCY(1000, KHZ)
 * // or
 * FREQUENCY(1000000, HZ)
 */
#define FREQUENCY(value, unit) ((value) * (unit))

// abstract Transmitter struct
struct Transmitter;

struct Transmitter *transmitter_create(double interval, uint64_t frequency);

void transmitter_destroy(struct Transmitter *self);

struct timespec transmitter_get_interval(struct Transmitter *self);

void transmitter_send_calibration(struct Transmitter *self, int length, int calibration_sequence[]);

void transmitter_send_letter(struct Transmitter *self, char letter);

clock_t transmitter_send_bit(struct Transmitter *self, int current_bit);

#endif // TRANSMITTER_H
