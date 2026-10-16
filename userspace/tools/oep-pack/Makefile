# oep-tools: pack/info/unpack/run (multi-call por argv0).
# SEM SIMD (regra do projeto): -mno-sse -mno-mmx -mgeneral-regs-only.
CC ?= gcc
CFLAGS = -O2 -Wall -Wextra -Werror -std=c99 -mno-sse -mno-mmx -mgeneral-regs-only
# inside-OvsbOS: estático (sem loader Linux no OvsbOS). Host pode usar
# `make HOST=1` (dinâmico).
ifeq ($(HOST),1)
STATIC =
else
STATIC = -static
endif

HDR = ../../libs/oep/oep.h ../../libs/oep/sha256.h
LIB = ../../libs/oep/sha256.c

all: oep-pack oep-info oep-unpack oep-run

oep-pack oep-info oep-unpack oep-run: oep.c $(HDR)
	$(CC) $(CFLAGS) $(STATIC) -o oep-pack     oep.c $(LIB)
	ln -sf oep-pack oep-info
	ln -sf oep-pack oep-unpack
	ln -sf oep-pack oep-run

clean:
	rm -f oep-pack oep-info oep-unpack oep-run

.PHONY: all clean
