CC = gcc
CFLAGS = -O2 -Wall -Wextra -Wno-unused-parameter -std=c99 \
         -I../../libs/pthread
LIB = ../../libs/pthread/libpthread.a
NAME = pthread_test
BINNAME = PTHREAD
DISK = ../../../disk.img

all: $(NAME)

$(LIB):
	$(MAKE) -C ../../libs/pthread

$(NAME): pthread_test.c $(LIB)
	$(CC) $(CFLAGS) -static -o $@ pthread_test.c $(LIB)

static: $(NAME)

install: static
	mcopy -o -i $(DISK) $(NAME) ::/BIN/$(BINNAME)

clean:
	rm -f $(NAME)
	$(MAKE) -C ../../libs/pthread clean

.PHONY: all static install clean
