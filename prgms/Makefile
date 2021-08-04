CFLAGS=-g

PRGMS=\
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
GM.SYS \

all: $(PRGMS)

install: mount $(PRGMS) umount

mount:
	@echo NOTICE: Sudo is required for loopback mounting. A password prompt may occur.
	@sudo mkdir -p /mnt/F1
	@-sudo mount -o loop,rw ../exampledisk.img /mnt/F1

$(PRGMS):
	@cd $(basename $(shell echo $@ | tr A-Z a-z)) && $(MAKE) install --no-print-directory

umount:
	@sudo umount /mnt/F1
	@sudo rmdir /mnt/F1

clean:
	rm */*.SYS
	rm */*.PRG
	rm */*.o
