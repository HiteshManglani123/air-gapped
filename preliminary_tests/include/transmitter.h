#ifndef TRANSMITTER_H
#define TRANSMITTER_H

#include <pthread.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define HIGH 1
#define LOW 0
#define MAX_CALIBRATION_LENGTH 8

// abstract Transmitter struct
struct Transmitter;

struct Transmitter *transmitter_create(double interval);

void transmitter_destroy(struct Transmitter *transmitter);

struct timespec transmitter_get_interval(struct Transmitter *transmitter);

void transmitter_send_calibration(struct Transmitter *transmitter, int length, int calibration_sequence[]);

void transmitter_send_letter(struct Transmitter *transmitter, char letter);

clock_t transmitter_send_bit(struct Transmitter *transmitter, int current_bit);

#endif // TRANSMITTER_H
