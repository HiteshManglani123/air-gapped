CC = gcc


CFLAGS = `sdl2-config --cflags` -pg -O3 -mfpu=neon -I/usr/local/include/libhackrf -I/usr/local/include/kissfft -I/usr/local/include
LDFLAGS = `sdl2-config --libs` -lSDL2_image -L/usr/local/lib
LIBS = -lhackrf -lkissfft-float -lfftw3 -lfftw3_threads -lpthread -lm



receiver: receiver.c
	$(CC) $(CFLAGS) -o receiver receiver.c secret_formula.c burger_stack_gui.c $(LDFLAGS) $(LIBS)

clean:
	rm -f receiver

