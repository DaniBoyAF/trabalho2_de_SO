CC = gcc
CFLAGS = -O2

LIBS = -fopenmp -pthread
PROGRAMA = mandelbrot

all:
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)
clean:
     del/Q mandelbrot.exe*.pgm time.txt 2>nul