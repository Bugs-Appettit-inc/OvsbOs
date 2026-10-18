CC = gcc
CFLAGS = -O2 -Wall -Wextra -Wno-unused-parameter -std=c99 -I.
TARGET = libminigl.a

all: $(TARGET)

$(TARGET): minigl.c minigl.h
	$(CC) $(CFLAGS) -c -o minigl.o minigl.c
	ar rcs $(TARGET) minigl.o

clean:
	rm -f minigl.o $(TARGET)

.PHONY: all clean
