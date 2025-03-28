#include <pthread.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>

#include <transmitter.h>

#define MESSAGE_SIZE 128

int main(int argc, char** argv) {
  struct Transmitter *transmitter = transmitter_create(0.3);

  char message[MESSAGE_SIZE];

  if (argc > 1)
  {
    if (strlen(argv[1]) < sizeof(message))
    {
      strcpy(message, argv[1]);
    }
  }
  else
  {
    printf("Input Message: ");
    fgets(message, sizeof(message), stdin);
  }

  transmitter_send_calibration(transmitter, 3, NULL);
  for (int i = 0; i<strlen(message); i++)
  {
    transmitter_send_letter(transmitter, message[i]);
  }


  transmitter_destroy(transmitter);

  return 0;
}

