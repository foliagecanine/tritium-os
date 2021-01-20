GCC=i686-elf-gcc
LIBDIR=../libc
CFLAGS=-m32 -Os -s -fno-pie -ffreestanding -nostartfiles

include $(LIBDIR)/make.config

OBJS=\
../libc/sys/sys.o\
main.o \
$(LIB_OBJS)\

.PHONY: all clean upload
.SUFFIXES: .o .c

all: FORK.PRG

FORK.PRG: $(OBJS) linker.ld
	ld -T linker.ld $(OBJS) -o FORK.PRG
	
.c.o:
	$(GCC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) -Iinclude -I$(LIBDIR)/include
	
dump: FORK.PRG
	objdump -D -b binary -mi386 FORK.PRG
	
upload: FORK.PRG
	sudo mkdir -p /mnt/F1
	sudo mount -o loop,rw ../../floppy.flp /mnt/F1
	sudo cp FORK.PRG /mnt/F1/prgms
	sudo umount /mnt/F1
	sudo rmdir /mnt/F1
	
clean:
	rm -f $(OBJS) *.o */*.o */*/*.objdump
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d
	rm FORK.PRG

install: FORK.PRG
	sudo cp FORK.PRG /mnt/F1/prgms
