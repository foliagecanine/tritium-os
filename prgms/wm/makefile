GCC=i686-tritium-gcc
CFLAGS?=
OUTPRG=WM.SYS
OUTFLDR=bin/os

.PHONY: all clean upload
.SUFFIXES: .o .c

OBJS=\
main.o \
gfunc.o \
draw.o \

all: $(OUTPRG)

$(OUTPRG): $(OBJS)
	$(GCC) $(CFLAGS) $(OBJS) -o $(OUTPRG)

.c.o: %.c
	$(GCC) $(CFLAGS) -c $< -o $@

dump: $(OUTPRG)
	i686-tritium-objdump -D $(OUTPRG)

upload: $(OUTPRG)
	sudo mkdir -p /mnt/F1
	sudo mount -o loop,rw ../../floppy.flp /mnt/F1
	sudo cp $(OUTPRG) /mnt/F1/$(OUTFLDR)
	sudo umount /mnt/F1
	sudo rmdir /mnt/F1

clean:
	rm $(OBJS)
	rm *.d
	rm $(OUTPRG)

install: $(OUTPRG)
	sudo cp $(OUTPRG) /mnt/F1/$(OUTFLDR)
