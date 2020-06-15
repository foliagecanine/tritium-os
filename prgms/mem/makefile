LIBDIR=../libc
CFLAGS=-m32 -Os -s -fno-pie -nostdlib -nostartfiles -fcf-protection=none -nodefaultlibs -fno-builtin

include $(LIBDIR)/make.config

OBJS=\
main.o \
$(LIB_OBJS)\

.PHONY: all clean upload
.SUFFIXES: .o .c

all: MEM.PRG

MEM.PRG: $(OBJS) linker.ld
	ld -T linker.ld $(OBJS) -o MEM.PRG
	
.c.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) -Iinclude -I$(LIBDIR)/include
	
dump: MEM.PRG
	objdump -D -b binary -mi386 MEM.PRG
	
upload: MEM.PRG
	sudo mkdir -p /mnt/F1
	sudo mount -o loop,rw ../../floppy.flp /mnt/F1
	sudo cp MEM.PRG /mnt/F1/bin
	sudo umount /mnt/F1
	sudo rmdir /mnt/F1
	
clean:
	rm -f $(OBJS) *.o */*.o */*/*.objdump
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d
	rm MEM.PRG
