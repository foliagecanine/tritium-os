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

all: HQ9COMP.PRG

HQ9COMP.PRG: $(OBJS) linker.ld
	ld -T linker.ld $(OBJS) -o HQ9COMP.PRG
	
.c.o:
	$(GCC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) -Iinclude -I$(LIBDIR)/include
	
dump: HQ9COMP.PRG
	objdump -D -b binary -mi386 HQ9COMP.PRG
	
upload: HQ9COMP.PRG
	sudo mkdir -p /mnt/F1
	sudo mount -o loop,rw ../../floppy.flp /mnt/F1
	sudo cp HQ9COMP.PRG /mnt/F1/prgms
	sudo umount /mnt/F1
	sudo rmdir /mnt/F1
	
clean:
	rm -f $(OBJS) *.o */*.o */*/*.objdump
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d
	rm HQ9COMP.PRG

install: HQ9COMP.PRG
	sudo cp HQ9COMP.PRG /mnt/F1/prgms
