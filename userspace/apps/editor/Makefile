CC = gcc
CFLAGS = -O2 -Wall -Wextra -Werror -std=c99 -I..
NAME = editor
BINNAME = EDITOR
DISK = ../../../disk.img

all: $(NAME)

$(NAME): main.c ../ovsb_fb.h ../font8x8.h
	$(CC) $(CFLAGS) -o $@ main.c

static: $(NAME)-st

$(NAME)-st: main.c ../ovsb_fb.h ../font8x8.h
	$(CC) $(CFLAGS) -static -o $@ main.c

install: static
	mcopy -o -i $(DISK) $(NAME)-st ::/BIN/$(BINNAME)

clean:
	rm -f $(NAME) $(NAME)-st

.PHONY: all static install clean
