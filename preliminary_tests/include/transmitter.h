#ifndef TRANSMITTER_H
#define TRANSMITTER_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "macros.h"

struct Transmitter {
  size_t number_of_virtual_cores;
  int interval;
  size_t number_of_pids;
  pid_t *pids;
};

struct Transmitter *transmitter_create(int interval);

void transmitter_destroy(struct Transmitter *transmitter);

void transmitter_send_letter(struct Transmitter *transmitter, char letter);

void transmitter_send_bit(struct Transmitter *transmitter, int current_bit);

int transmitter_send_high(struct Transmitter *transmitter);

void transmitter_send_low(struct Transmitter *transmitter);

#endif // TRANSMITTER_H
