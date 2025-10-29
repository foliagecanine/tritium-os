GCC=i686-tritium-gcc
CFLAGS?=
OUTPRG=HQ9COMP.PRG
OUTFLDR=prgms

.PHONY: all clean upload
.SUFFIXES: .o .c

all: $(OUTPRG)

$(OUTPRG): main.c
	$(GCC) $(CFLAGS) main.c -o $(OUTPRG)

dump: $(OUTPRG)
	i686-tritium-objdump -D $(OUTPRG)
	
clean:
	rm *.o
	rm *.d
	rm $(OUTPRG)

install: $(OUTPRG)
	mcopy -o -i ../../exampledisk.img $(OUTPRG) ::$(OUTFLDR)/$(OUTPRG)
