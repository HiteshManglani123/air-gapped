#include <pthread.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "../include/macros.h"
#include "../include/transmitter.h"

int main(void) {
  // A = 01000001

  pid_t pids[NUM_OF_CORES];
  int interval = 1 * CLOCKS_PER_SEC;

  struct Transmitter *transmitter = transmitter_create(interval);

  transmitter_send_calibration(transmitter, 4);

  transmitter_send_letter(transmitter, 'H');
  transmitter_send_letter(transmitter, 'e');
  transmitter_send_letter(transmitter, 'l');
  transmitter_send_letter(transmitter, 'l');
  transmitter_send_letter(transmitter, 'o');

  // calibration
  /*
  transmitter_send_high(transmitter);
  transmitter_send_low(transmitter);
  transmitter_send_high(transmitter);

  // binary A
  transmitter_send_low(transmitter);
  transmitter_send_high(transmitter);
  transmitter_send_low(transmitter);
  transmitter_send_low(transmitter);
  transmitter_send_low(transmitter);
  transmitter_send_low(transmitter);
  transmitter_send_low(transmitter);
  transmitter_send_high(transmitter);
  */

  transmitter_destroy(transmitter);

  return 0;
}
