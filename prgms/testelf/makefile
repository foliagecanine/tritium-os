all: TESTELF.PRG

TESTELF.PRG:
	nasm -felf32 testelf.asm -o testelf.o
	ld -m elf_i386 testelf.o -o TESTELF.PRG

upload: TESTPRG.PRG
	sudo mkdir -p /mnt/F1
	sudo mount -o loop,rw ../../floppy.flp /mnt/F1
	sudo cp TESTELF.PRG /mnt/F1/prgms
	sudo umount /mnt/F1
	sudo rmdir /mnt/F1

install: TESTELF.PRG
	sudo cp TESTELF.PRG /mnt/F1/prgms
