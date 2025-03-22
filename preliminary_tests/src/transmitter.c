#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// custom
#include <transmitter.h>
#include <macros.h>

static int _get_number_of_virtual_cores(int number_of_physical_cores);
static void _calculate_primes(struct timespec interval);
static int _get_big_endian_bit_from_letter_by_index(char letter, int index_from_most_significant_bit);
static int _send_high(struct Transmitter *transmitter);
static void _send_low(struct Transmitter *transmitter);
static void _seconds_to_timespec(double seconds, struct timespec *ts);
static int _timespec_less_than(const struct timespec *ts1, const struct timespec *ts2);
static struct timespec _diff_timespec(struct timespec start, struct timespec end);

struct Transmitter {
  size_t number_of_virtual_cores;
  struct timespec interval;
  size_t number_of_pids;
  pid_t *pids;
};

struct Transmitter *transmitter_create(double interval) {

  struct Transmitter *transmitter = malloc(sizeof(struct Transmitter));

  transmitter->number_of_virtual_cores = _get_number_of_virtual_cores(sysconf(_SC_NPROCESSORS_ONLN));

  transmitter->number_of_pids = transmitter->number_of_virtual_cores;

  transmitter->pids = malloc(sizeof(pid_t) * transmitter->number_of_pids);

  _seconds_to_timespec(interval, &transmitter->interval);

  return transmitter;
}

void transmitter_destroy(struct Transmitter *self) {
  free(self->pids);
  free(self);
}

struct timespec transmitter_get_interval(struct Transmitter *self)
{
  return self->interval;
}

void transmitter_send_letter(struct Transmitter *self, char letter) {
  printf("Transmitting: %c\n", letter);
  for (int index = CHAR_BIT - 1; index >= 0; index--) {
    int current_bit = _get_big_endian_bit_from_letter_by_index(letter, index);
    printf("%d", current_bit);
    fflush(stdout);
    transmitter_send_bit(self, current_bit);
  }
  printf("\n");
}

void transmitter_send_calibration(struct Transmitter *self, int length, int calibration_sequence[]) {
  int current_bit;
  int calibration_index = 0;

  int length_is_an_odd = length % 2;

  // ensure the calibration starts with high
  do {
    if (length_is_an_odd) {
      current_bit = (length % 2);
    }
    else {
      current_bit = (length % 2) ^ 1;
    }
    printf("%d", current_bit);
    fflush(stdout);
    if (calibration_sequence != NULL) {
      calibration_sequence[calibration_index] = current_bit;
    }
    calibration_index++;
    transmitter_send_bit(self, current_bit);
  } while (length--);
  printf("\n");
}

clock_t transmitter_send_bit(struct Transmitter *self, int current_bit) {
  clock_t runtime;
  if (current_bit == 0) {
    clock_t start = clock();
    _send_low(self);
    runtime = (double)(clock() - start);
  } else if (current_bit == 1) {
    clock_t start = clock();
    _send_high(self);
    runtime = (double)(clock() - start);
  } else {
    abort();
  }

  return runtime;
}

static int _send_high(struct Transmitter *self) {
  unsigned long i;

  for (i = 0; i < self->number_of_pids; ++i) {
    if (!(self->pids[i] = fork())) {
      _calculate_primes(self->interval);
      exit(0);
    }
    if (self->pids[i] < 0) {
      perror("Fork");
      exit(1);
    }
  }

  for (i = 0; i < self->number_of_pids; ++i) {
    waitpid(self->pids[i], NULL, 0);
  }

  return 0;
}

static void _send_low(struct Transmitter *self) {
  nanosleep(&self->interval, NULL);
}

static void _calculate_primes(struct timespec interval) {
  unsigned long i, num, primes = 0;
  clock_t start;
  clock_t end;

  struct timespec TimeSpecStart;
  struct timespec TimeSpecEnd;
  struct timespec TimeSpecTotalTimeTaken;

  clock_gettime(0, &TimeSpecStart);
  for (num = 1; num <= MAX_PRIME; ++num) {
    for (i = 2; (i <= num) && (num % i != 0); ++i)
      ;
    if (i == num) {
      ++primes;
    }
    clock_gettime(0, &TimeSpecEnd);
    TimeSpecTotalTimeTaken = _diff_timespec(TimeSpecStart, TimeSpecEnd);

    if ( _timespec_less_than(&interval, &TimeSpecTotalTimeTaken)) {
      return;
    }
  }
}

static int _get_number_of_virtual_cores(int number_of_physical_cores) {
  return number_of_physical_cores * 2;
}

static int _get_big_endian_bit_from_letter_by_index(char letter, int index_from_most_significant_bit) {
  int current_bit = (letter >> index_from_most_significant_bit) & 1;
  return current_bit;
}

void _seconds_to_timespec(double seconds, struct timespec *ts) {
    ts->tv_sec = (time_t)floor(seconds);
    ts->tv_nsec = (long)((seconds - ts->tv_sec) * 1e9);
}

int _timespec_less_than(const struct timespec *ts1, const struct timespec *ts2) {
    return (ts1->tv_sec < ts2->tv_sec) || 
           (ts1->tv_sec == ts2->tv_sec && ts1->tv_nsec < ts2->tv_nsec);
}

struct timespec _diff_timespec(struct timespec start, struct timespec end) {
    struct timespec result;

    // Calculate the difference in seconds and nanoseconds
    result.tv_sec = end.tv_sec - start.tv_sec;
    result.tv_nsec = end.tv_nsec - start.tv_nsec;

    // Handle cases where nanoseconds are negative (borrow 1 second)
    if (result.tv_nsec < 0) {
        result.tv_sec -= 1;
        result.tv_nsec += 1000000000; // Add 1 billion nanoseconds
    }

    return result;
}
