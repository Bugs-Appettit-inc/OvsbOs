CC = gcc
CFLAGS = -O2 -Wall -Wextra -Wno-unused-parameter -std=c99 -I.
TARGET = libdl.a

all: $(TARGET)

$(TARGET): dl.c dl.h
	$(CC) $(CFLAGS) -c -o dl.o dl.c
	ar rcs $(TARGET) dl.o

clean:
	rm -f dl.o $(TARGET)

.PHONY: all clean
