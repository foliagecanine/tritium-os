CFLAGS=-g

PRGMS=\
IDLE.SYS \
SHELL.SYS \
MEM.PRG \
CAT.PRG \
ADVNTURE.PRG \
LS.PRG \
GUI.SYS \
EDIT.PRG \
KEYREAD.PRG \
FORK.PRG \
ASM.PRG \

all: $(PRGMS)

install: mount $(PRGMS) umount

mount:
	@sudo mkdir -p /mnt/F1
	@-sudo mount -o loop,rw ../floppy.flp /mnt/F1

$(PRGMS):
	@cd $(basename $(shell echo $@ | tr A-Z a-z)) && $(MAKE) install --no-print-directory

umount:
	@sudo umount /mnt/F1
	@sudo rmdir /mnt/F1

clean:
	rm */*.SYS
	rm */*.PRG
	rm */*.o
