CC = gcc
CFLAGS = -O2 -Wall -Wextra -Werror -std=c99 -mno-sse -mno-mmx -mgeneral-regs-only

all: oasm omake oman

oasm: oasm.c
	$(CC) $(CFLAGS) -o $@ oasm.c

omake: omake.c
	$(CC) $(CFLAGS) -o $@ omake.c

oman: oman.c
	$(CC) $(CFLAGS) -o $@ oman.c

clean:
	rm -f oasm omake oman

.PHONY: all clean
