CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c99 -fPIC -shared

all: libanswer.so

libanswer.so: answer.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f libanswer.so

.PHONY: all clean
