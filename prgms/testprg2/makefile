all: testprg2.asm
	nasm -fbin testprg2.asm -o TESTPRG2.PRG

upload: TESTPRG2.PRG
	sudo mkdir -p /mnt/F1
	sudo mount -o loop,rw ../../floppy.flp /mnt/F1
	sudo cp TESTPRG2.PRG /mnt/F1/prgms
	sudo umount /mnt/F1
	sudo rmdir /mnt/F1
