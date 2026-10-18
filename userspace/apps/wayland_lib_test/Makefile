CC = gcc
CFLAGS = -O2 -Wall -Wextra -Wno-unused-parameter -std=c99 \
         -I ../libs/dl -I /home/gabicnt/OvsbOS/wl-build/wayland/include
WL_DIR := /home/gabicnt/OvsbOS/wl-build/wayland
LIBFFI := /home/gabicnt/OvsbOS/wl-build/libffi/libffi.a
NAME = wayland_lib_test
BINNAME = WAYLAND_LIB_TEST
DISK = ../../../disk.img

all: $(NAME)

$(NAME): wayland_lib_test.c $(WL_DIR)/libwayland-client.a \
         $(WL_DIR)/libwayland-util.a $(LIBFFI)
	$(CC) $(CFLAGS) -static -o $@ wayland_lib_test.c \
		$(WL_DIR)/libwayland-client.a $(WL_DIR)/libwayland-util.a \
		$(WL_DIR)/libwayland-private.a $(LIBFFI) \
		-lpthread -lrt -ldl -lm

static: $(NAME)

install: static
	mcopy -o -i $(DISK) $(NAME) ::/BIN/$(BINNAME)

clean:
	rm -f $(NAME)

.PHONY: all static install clean
