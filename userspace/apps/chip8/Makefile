CC = gcc
CFLAGS = -O2 -Wall -Wextra -Werror -std=c99 -I.. -DIMG_WITH_PNG
NAME = chip8
BINNAME = CHIP8
DISK = ../../../disk.img

all: $(NAME)

$(NAME): main.c ../ovsb_fb.h ../font8x8.h
	$(CC) $(CFLAGS) -o $@ main.c

static: $(NAME)-st

$(NAME)-st: main.c ../ovsb_fb.h ../font8x8.h
	$(CC) $(CFLAGS) -static -o $@ main.c

install: static
	mcopy -o -i $(DISK) $(NAME)-st ::/BIN/$(BINNAME)
	mcopy -o -i $(DISK) roms/test_draw.ch8 ::/APPS/CHIP8_DRAW.C8
	mcopy -o -i $(DISK) roms/test_bounce.ch8 ::/APPS/CHIP8_BOUN.C8

clean:
	rm -f $(NAME) $(NAME)-st

.PHONY: all static install clean
