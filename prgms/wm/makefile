GCC=i686-tritium-gcc
CFLAGS:=-O3 -ftree-vectorize -fopt-info
OUTPRG=WM.SYS
OUTFLDR=bin/os

.PHONY: all clean upload
.SUFFIXES: .o .c

OBJS=\
main.o \
gfunc.o \
draw.o \

all: $(OUTPRG)

$(OUTPRG): $(OBJS)
	$(GCC) $(CFLAGS) $(OBJS) -o $(OUTPRG)

.c.o: %.c
	$(GCC) $(CFLAGS) -c $< -o $@

dump: $(OUTPRG)
	i686-tritium-objdump -D $(OUTPRG)

clean:
	rm $(OBJS)
	rm *.d
	rm $(OUTPRG)

install: $(OUTPRG)
	mcopy -o -i ../../exampledisk.img $(OUTPRG) ::$(OUTFLDR)/$(OUTPRG)
