CC = gcc
CFLAGS = -O2 -Wall -Wextra -Werror -std=c99 -mno-sse -mno-mmx -mgeneral-regs-only -DIMG_WITH_PNG
AR = ar

LIB = libimage.a
SRC = image.c png.c
OBJ = image.o png.o
TEST = imgtest

all: $(LIB) $(TEST)

$(LIB): $(OBJ)
	$(AR) rcs $@ $(OBJ)

image.o: image.c image.h
	$(CC) $(CFLAGS) -c -o $@ image.c

png.o: png.c image.h
	$(CC) $(CFLAGS) -c -o $@ png.c

$(TEST): test.c $(LIB)
	$(CC) $(CFLAGS) -o $@ test.c $(LIB)

clean:
	rm -f $(LIB) $(OBJ) $(TEST)

.PHONY: all clean
