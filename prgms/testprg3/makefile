all: TESTPRG3.PRG

TESTPRG3.PRG:
	nasm -fbin testprg3.asm -o TESTPRG3.PRG

upload: TESTPRG3.PRG
	sudo mkdir -p /mnt/F1
	sudo mount -o loop,rw ../../floppy.flp /mnt/F1
	sudo cp TESTPRG3.PRG /mnt/F1/prgms
	sudo umount /mnt/F1
	sudo rmdir /mnt/F1

install: TESTPRG3.PRG
	sudo cp TESTPRG3.PRG /mnt/F1/prgms
