all: TESTPRG.PRG

TESTPRG.PRG:
	nasm -fbin testprg.asm -o TESTPRG.PRG

upload: TESTPRG.PRG
	sudo mkdir -p /mnt/F1
	sudo mount -o loop,rw ../../floppy.flp /mnt/F1
	sudo cp TESTPRG.PRG /mnt/F1/prgms
	sudo umount /mnt/F1
	sudo rmdir /mnt/F1

install: TESTPRG.PRG
	sudo cp TESTPRG.PRG /mnt/F1/prgms
