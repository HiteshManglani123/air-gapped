CC = gcc
CFLAGS = -I/usr/local/include
LDFLAGS = -L/usr/local/lib -lfftw3 -lm -lhackrf

receiver: receiver.c
	$(CC) $(CFLAGS) -o receiver receiver.c $(LDFLAGS)

clean:
	rm -f receiver
