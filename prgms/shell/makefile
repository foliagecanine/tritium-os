LIBDIR=../libc
CFLAGS=-m32 -Os -s -fno-pie -nostdlib -nostartfiles -fcf-protection=none -nodefaultlibs -fno-builtin

include $(LIBDIR)/make.config

OBJS=\
main.o \
$(LIB_OBJS)\

.PHONY: all clean upload
.SUFFIXES: .o .c

all: SHELL.SYS

SHELL.SYS: $(OBJS) linker.ld
	ld -T linker.ld $(OBJS) -o SHELL.SYS
	
.c.o:
	$(GCC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) -Iinclude -I$(LIBDIR)/include
	
dump: SHELL.SYS
	objdump -D -b binary -mi386 SHELL.SYS
	
upload: SHELL.SYS
	sudo mkdir -p /mnt/F1
	sudo mount -o loop,rw ../../floppy.flp /mnt/F1
	sudo cp SHELL.SYS /mnt/F1/bin
	sudo umount /mnt/F1
	sudo rmdir /mnt/F1
	
clean:
	rm -f $(OBJS) *.o */*.o */*/*.objdump
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d
	rm SHELL.SYS

install: SHELL.SYS
	sudo cp SHELL.SYS /mnt/F1/bin
