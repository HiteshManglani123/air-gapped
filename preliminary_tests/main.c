#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define NUM_OF_CORES 8
// #define MAX_PRIME 50000
#define MAX_PRIME 100000000

#define UP true
#define DOWN false

void do_primes(int interval_in_ms) {
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
    if ((end - start) >= interval_in_ms) {
      printf("Took: %lums\n", end - start);
      return;
    }
  }
  // printf("Calculated %lu primes.\n", primes);
}

clock_t send(pid_t pids[NUM_OF_CORES], int interval) {
  clock_t start, end;
  clock_t run_time;
  unsigned long i;

  start = clock();
  for (i = 0; i < NUM_OF_CORES; ++i) {
    if (!(pids[i] = fork())) {
      do_primes(interval);
      exit(0);
    }
    if (pids[i] < 0) {
      perror("Fork");
      exit(1);
    }
  }

  for (i = 0; i < NUM_OF_CORES; ++i) {
    waitpid(pids[i], NULL, 0);
  }
  end = clock();

  return end - start;

  // return 0;
}

void s(int interval) { usleep(interval * 2); }

typedef void *(*start_routine)(void *);
int main(void) {
  clock_t run_time;
  // A = 01000001

  pid_t pids[NUM_OF_CORES];
  int interval = 1 * CLOCKS_PER_SEC;

  s(interval);
  send(pids, interval);

  s(interval);
  send(pids, interval);
  s(interval);

  send(pids, interval);
  s(interval);
  s(interval);
  s(interval);
  s(interval);
  s(interval);
  send(pids, interval);

  return 0;
}
