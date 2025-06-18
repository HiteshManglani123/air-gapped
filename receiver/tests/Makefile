CC = gcc

CFLAGS = -pg -O3 -mfpu=neon -I/usr/local/include/libhackrf -I/usr/local/include/kissfft -I/usr/local/include
LDFLAGS = -L/usr/local/lib
LIBS = -lkissfft-float -lfftw3 -lfftw3_threads -lpthread -lm

test_receiver: test_receiver.c
	$(CC) $(CFLAGS) -o test_receiver test_receiver.c ../secret_formula.c $(LDFLAGS) $(LIBS)

test: test_receiver
	./test_receiver

clean:
	rm -f test_receiver 