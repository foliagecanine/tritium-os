GCC=i686-tritium-gcc
CFLAGS=
OUTPRG=FORK.PRG
OUTFLDR=prgms

.PHONY: all clean upload
.SUFFIXES: .o .c

all: $(OUTPRG)

$(OUTPRG):
	$(GCC) $(CFLAGS) main.c -o $(OUTPRG)

dump: FORK.PRG
	i686-tritium-objdump -D $(OUTPRG)
	
upload: FORK.PRG
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
	sudo cp $(OUTPRG) /mnt/F1/$(OUTFLDR)
