CC = gcc
CFLAGS = `sdl2-config --cflags` -pg -O3 -mfpu=neon -I/usr/local/include/libhackrf -I/usr/local/include/kissfft
LDFLAGS = `sdl2-config --libs` -lSDL2_image -L/usr/local/lib
LIBS = -lhackrf -lkissfft-float -lfftw3 -lfftw3_threads -lpthread -lm

TARGET = receiver

all: $(TARGET)

$(TARGET): receiver.c burger_stack_gui.c secret_formula.c
	$(CC) $(CFLAGS) $^ $(LDFLAGS) $(LIBS) -o $@

clean:
	rm -f $(TARGET)
