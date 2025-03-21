#ifndef TRANSMITTER_H
#define TRANSMITTER_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#include "macros.h"

#define HIGH 1
#define LOW 0

#define NANOSECOND_MULTIPLIER

struct Transmitter {
  size_t number_of_virtual_cores;
  struct timespec interval;
  size_t number_of_pids;
  pid_t *pids;
};

struct Transmitter *transmitter_create(double interval);

void transmitter_destroy(struct Transmitter *transmitter);

void transmitter_send_calibration(struct Transmitter *transmitter, int length);

void transmitter_send_letter(struct Transmitter *transmitter, char letter);

clock_t transmitter_send_bit(struct Transmitter *transmitter, int current_bit);

#endif // TRANSMITTER_H
