GCC=i686-tritium-gcc
CFLAGS?=
OUTPRG=MEM.PRG
OUTFLDR=bin

.PHONY: all clean upload
.SUFFIXES: .o .c

all: $(OUTPRG)

$(OUTPRG): main.c
	$(GCC) $(CFLAGS) main.c -o $(OUTPRG)

dump: $(OUTPRG)
	i686-tritium-objdump -D $(OUTPRG)

upload: $(OUTPRG)
	sudo mkdir -p /mnt/F1
	sudo mount -o loop,rw ../../floppy.flp /mnt/F1
	sudo cp $(OUTPRG) /mnt/F1/$(OUTFLDR)
	sudo umount /mnt/F1
	sudo rmdir /mnt/F1

clean:
	rm *.o
	rm *.d
	rm $(OUTPRG)

install: $(OUTPRG)
	mcopy -o -i ../../exampledisk.img $(OUTPRG) ::$(OUTFLDR)/$(OUTPRG)
