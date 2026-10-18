CC = gcc
CFLAGS = -O2 -Wall -Wextra -Wno-unused-parameter -std=c99 \
         -I../../libs/minigl
LIB = ../../libs/minigl/libminigl.a
NAME = minigl_test
BINNAME = MINIGL_TEST
DISK = ../../../disk.img

all: $(NAME)

$(LIB):
	$(MAKE) -C ../../libs/minigl

$(NAME): minigl_test.c $(LIB)
	$(CC) $(CFLAGS) -static -o $@ minigl_test.c $(LIB)

static: $(NAME)

install: static
	mcopy -o -i $(DISK) $(NAME) ::/BIN/$(BINNAME)

clean:
	rm -f $(NAME)
	$(MAKE) -C ../../libs/minigl clean

.PHONY: all static install clean
