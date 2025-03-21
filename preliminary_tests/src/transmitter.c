#include <limits.h>
#include <transmitter.h>

static int _get_number_of_virtual_cores(int number_of_physical_cores);
static void _calculate_primes(int interval_in_ms);
static int
_get_big_endian_bit_from_letter_by_index(char letter,
                                         int index_from_most_significant_bit);
static int _send_high(struct Transmitter *transmitter);
static void _send_low(struct Transmitter *transmitter);

struct Transmitter *transmitter_create(int interval) {

  struct Transmitter *transmitter = malloc(sizeof(struct Transmitter));

  transmitter->number_of_virtual_cores =
      _get_number_of_virtual_cores(sysconf(_SC_NPROCESSORS_ONLN));

  transmitter->number_of_pids = transmitter->number_of_virtual_cores;

  transmitter->pids = malloc(sizeof(pid_t) * transmitter->number_of_pids);

  transmitter->interval = interval;

  return transmitter;
}

void transmitter_destroy(struct Transmitter *transmitter) {
  free(transmitter->pids);
  free(transmitter);
}

void transmitter_send_letter(struct Transmitter *transmitter, char letter) {
  printf("Transmitting: %c\n", letter);
  for (int index = CHAR_BIT - 1; index >= 0; index--) {
    int current_bit = _get_big_endian_bit_from_letter_by_index(letter, index);
    printf("%d", current_bit);
    fflush(stdout);
    transmitter_send_bit(transmitter, current_bit);
  }
  printf("\n");
}

void transmitter_send_calibration(struct Transmitter *transmitter, int length) {
  while (length--) {
    printf("%d", length % 2);
    fflush(stdout);
    transmitter_send_bit(transmitter, (length % 2));
  }
  printf("\n");
}

void transmitter_send_bit(struct Transmitter *transmitter, int current_bit) {
  if (current_bit == 0) {
    _send_low(transmitter);
  } else if (current_bit == 1) {
    _send_high(transmitter);
  } else {
    abort();
  }
}

static int _send_high(struct Transmitter *transmitter) {
  unsigned long i;

  for (i = 0; i < transmitter->number_of_pids; ++i) {
    if (!(transmitter->pids[i] = fork())) {
      _calculate_primes(transmitter->interval);
      exit(0);
    }
    if (transmitter->pids[i] < 0) {
      perror("Fork");
      exit(1);
    }
  }

  for (i = 0; i < transmitter->number_of_pids; ++i) {
    waitpid(transmitter->pids[i], NULL, 0);
  }

  return 0;
}

static void _send_low(struct Transmitter *transmitter) {
  usleep(transmitter->interval * 2);
}

static void _calculate_primes(int interval) {
  unsigned long i, num, primes = 0;
  clock_t start;
  clock_t end;

  start = clock();
  for (num = 1; num <= MAX_PRIME; ++num) {
    for (i = 2; (i <= num) && (num % i != 0); ++i)
      ;
    if (i == num) {
      ++primes;
    }
    end = clock();
    if ((end - start) >= interval) {
      return;
    }
  }
}

static int _get_number_of_virtual_cores(int number_of_physical_cores) {
  return number_of_physical_cores * 2;
}

static int
_get_big_endian_bit_from_letter_by_index(char letter,
                                         int index_from_most_significant_bit) {
  int current_bit = (letter >> index_from_most_significant_bit) & 1;
  return current_bit;
}
