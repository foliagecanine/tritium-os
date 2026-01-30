GCC=i686-tritium-gcc
CFLAGS?=
OUTPRG=ASM.PRG
OUTFLDR=prgms

.PHONY: all clean upload
.SUFFIXES: .o .c

OBJS=\
main.o \
encode.o \
estring.o \
x86ops.o \
macros.o \
elf.o \

all: $(OUTPRG)

$(OUTPRG): $(OBJS)
	$(GCC) $(CFLAGS) $(OBJS) -o $(OUTPRG)

.c.o: %.c
	$(GCC) $(CFLAGS) -c $< -o $@

dump: $(OUTPRG)
	i686-tritium-objdump -D $(OUTPRG)
	
clean:
	rm $(OBJS)
	rm $(OUTPRG)

install: $(OUTPRG)
	mcopy -o -i ../../exampledisk.img $(OUTPRG) ::$(OUTFLDR)/$(OUTPRG)
