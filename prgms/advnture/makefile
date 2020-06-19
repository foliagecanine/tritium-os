GCC=gcc
LIBDIR=../libc
CFLAGS=-m32 -Os -s -fno-pie -nostdlib -nostartfiles -fcf-protection=none -nodefaultlibs -fno-builtin

include $(LIBDIR)/make.config

OBJS=\
main.o \
$(LIB_OBJS)\

.PHONY: all clean upload
.SUFFIXES: .o .c

all: ADVNTURE.PRG

ADVNTURE.PRG: $(OBJS) linker.ld
	ld -T linker.ld $(OBJS) -o ADVNTURE.PRG
	
.c.o:
	$(GCC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) -Iinclude -I$(LIBDIR)/include
	
dump: ADVNTURE.PRG
	objdump -D -b binary -mi386 ADVNTURE.PRG
	
upload: ADVNTURE.PRG
	sudo mkdir -p /mnt/F1
	sudo mount -o loop,rw ../../floppy.flp /mnt/F1
	sudo cp ADVNTURE.PRG /mnt/F1/prgms
	sudo umount /mnt/F1
	sudo rmdir /mnt/F1
	
clean:
	rm -f $(OBJS) *.o */*.o */*/*.objdump
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d
	rm ADVNTURE.PRG

install: ADVNTURE.PRG
	sudo cp ADVNTURE.PRG /mnt/F1/prgms
