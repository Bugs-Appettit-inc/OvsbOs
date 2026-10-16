CC = gcc
CFLAGS = -O2 -Wall -Wextra -Werror -std=c99 -mno-sse -mno-mmx -mgeneral-regs-only

all: oep-pack oep-unpack oep-list oep-run

oep-pack: oep.c
	$(CC) $(CFLAGS) -o $@ oep.c

oep-unpack: oep.c
	$(CC) $(CFLAGS) -o $@ oep.c

oep-list: oep.c
	$(CC) $(CFLAGS) -o $@ oep.c

oep-run: oep.c
	$(CC) $(CFLAGS) -o $@ oep.c

clean:
	rm -f oep-pack oep-unpack oep-list oep-run

.PHONY: all clean
