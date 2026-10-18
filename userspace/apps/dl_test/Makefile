CC = gcc
CFLAGS = -O2 -Wall -Wextra -Wno-unused-parameter -std=c99 \
         -I../../libs/dl
LIBDL = ../../libs/dl/libdl.a
LIBANSWER = ../../libs/libanswer/libanswer.so
NAME = dl_test
BINNAME = DL_TEST
DISK = ../../../disk.img

all: $(NAME)

$(LIBDL):
	$(MAKE) -C ../../libs/dl

$(LIBANSWER):
	$(MAKE) -C ../../libs/libanswer

$(NAME): dl_test.c $(LIBDL)
	$(CC) $(CFLAGS) -static -o $@ dl_test.c $(LIBDL)

static: $(NAME)

install: static $(LIBANSWER)
	mcopy -o -i $(DISK) $(NAME) ::/BIN/$(BINNAME)
	mcopy -o -i $(DISK) $(LIBANSWER) ::/BIN/libanswer.so

clean:
	rm -f $(NAME)
	$(MAKE) -C ../../libs/dl clean
	$(MAKE) -C ../../libs/libanswer clean

.PHONY: all static install clean
