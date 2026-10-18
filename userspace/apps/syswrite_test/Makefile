CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c99
NAME = syswrite_test
BINNAME = SWRITE_TEST
DISK = ../../../disk.img

all: $(NAME)

$(NAME): syswrite_test.c
	$(CC) $(CFLAGS) -static -o $@ syswrite_test.c

install: $(NAME)
	mcopy -o -i $(DISK) $(NAME) ::/BIN/$(BINNAME)

clean:
	rm -f $(NAME)

.PHONY: all install clean
