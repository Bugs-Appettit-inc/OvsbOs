CC = gcc
CFLAGS = -O2 -Wall -Wextra -Werror -Wno-format-truncation -std=c99 -I.. -DIMG_WITH_PNG
IMGDIR = ../../libs/image
IMGLIB = ../../libs/image/libimage.a
NAME = fm
BINNAME = FM
DISK = ../../../disk.img

all: $(NAME)

$(NAME): main.c ../ovsb_fb.h ../font8x8.h $(IMGLIB)
	$(CC) $(CFLAGS) -o $@ main.c $(IMGLIB)

static: $(NAME)-st

$(NAME)-st: main.c ../ovsb_fb.h ../font8x8.h $(IMGLIB)
	$(CC) $(CFLAGS) -static -o $@ main.c $(IMGLIB)

install: static
$(IMGLIB):
	$(MAKE) -C $(IMGDIR)
	mcopy -o -i $(DISK) $(NAME)-st ::/BIN/$(BINNAME)

clean:
	rm -f $(NAME) $(NAME)-st

.PHONY: all static install clean
