CC = gcc
CFLAGS = -O2 -Wall -Wextra -Wno-unused-parameter -std=c99 -I.
TARGET = libpthread.a

all: $(TARGET)

$(TARGET): pthread.c pthread.h
	$(CC) $(CFLAGS) -c -o pthread.o pthread.c
	ar rcs $(TARGET) pthread.o

clean:
	rm -f pthread.o $(TARGET)

.PHONY: all clean
