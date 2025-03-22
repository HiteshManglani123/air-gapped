#include <pthread.h>
#include <sys/wait.h>
#include <unistd.h>

#include <transmitter.h>

int main(void) {
  // A = 01000001

  struct Transmitter *transmitter = transmitter_create(0.3);

  transmitter_send_calibration(transmitter, 5, NULL);

  transmitter_send_letter(transmitter, 'H');
  transmitter_send_letter(transmitter, 'e');
  transmitter_send_letter(transmitter, 'l');
  transmitter_send_letter(transmitter, 'l');
  transmitter_send_letter(transmitter, 'o');

  transmitter_destroy(transmitter);

  return 0;
}
