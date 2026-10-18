CC = gcc
CFLAGS = -O2 -Wall -Wextra -Wno-unused-parameter -std=c99 \
         -I../../libs/pthread
LIB = ../../libs/pthread/libpthread.a
NAME = pthread_test
BINNAME = PTHREAD
MIN_NAME = pthread_min
MIN_BINNAME = PTHREAD.MN
DISK = ../../../disk.img

all: $(NAME) $(MIN_NAME)

$(LIB):
	$(MAKE) -C ../../libs/pthread

$(NAME): pthread_test.c $(LIB)
	$(CC) $(CFLAGS) -static -o $@ pthread_test.c $(LIB)

$(MIN_NAME): pthread_min.c $(LIB)
	$(CC) $(CFLAGS) -static -o $@ pthread_min.c $(LIB)

static: $(NAME) $(MIN_NAME)

install: static
	mcopy -o -i $(DISK) $(NAME) ::/BIN/$(BINNAME)
	mcopy -o -i $(DISK) $(MIN_NAME) ::/BIN/$(MIN_BINNAME)

clean:
	rm -f $(NAME)
	$(MAKE) -C ../../libs/pthread clean

.PHONY: all static install clean
