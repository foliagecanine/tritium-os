GCC=gcc
LIBDIR=../libc
CFLAGS=-m32 -Os -s -fno-pie -nostdlib -nostartfiles -fcf-protection=none -nodefaultlibs -fno-builtin

include $(LIBDIR)/make.config

OBJS=\
../libc/sys/sys.o\
main.o \
$(LIB_OBJS)\

.PHONY: all clean upload
.SUFFIXES: .o .c

all: KEYREAD.PRG

KEYREAD.PRG: $(OBJS) linker.ld
	ld -T linker.ld $(OBJS) -o KEYREAD.PRG
	
.c.o:
	$(GCC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) -Iinclude -I$(LIBDIR)/include
	
dump: KEYREAD.PRG
	objdump -D -b binary -mi386 KEYREAD.PRG
	
upload: KEYREAD.PRG
	sudo mkdir -p /mnt/F1
	sudo mount -o loop,rw ../../floppy.flp /mnt/F1
	sudo cp KEYREAD.PRG /mnt/F1/PRGMS
	sudo umount /mnt/F1
	sudo rmdir /mnt/F1
	
clean:
	rm -f $(OBJS) *.o */*.o */*/*.objdump
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d
	rm KEYREAD.PRG

install: KEYREAD.PRG
	sudo cp KEYREAD.PRG /mnt/F1/PRGMS
