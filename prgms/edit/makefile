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

all: EDIT.PRG

EDIT.PRG: $(OBJS) linker.ld
	ld -T linker.ld $(OBJS) -o EDIT.PRG
	
.c.o:
	$(GCC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) -Iinclude -I$(LIBDIR)/include
	
dump: EDIT.PRG
	objdump -D -b binary -mi386 EDIT.PRG
	
upload: EDIT.PRG
	sudo mkdir -p /mnt/F1
	sudo mount -o loop,rw ../../floppy.flp /mnt/F1
	sudo cp EDIT.PRG /mnt/F1/bin
	sudo umount /mnt/F1
	sudo rmdir /mnt/F1
	
clean:
	rm -f $(OBJS) *.o */*.o */*/*.objdump
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d
	rm EDIT.PRG

install: EDIT.PRG
	sudo cp EDIT.PRG /mnt/F1/bin
