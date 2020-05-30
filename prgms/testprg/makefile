all: testprg.asm
	nasm -fbin testprg.asm -o TESTPRG.PRG

upload: TESTPRG.PRG
	sudo mkdir -p /mnt/F1
	sudo mkdir -p /mnt/F2
	sudo mount -o loop,rw ../../floppy.flp /mnt/F1
	sudo mount -o loop,rw ../../floppy2.flp /mnt/F2
	sudo cp TESTPRG.PRG /mnt/F1
	sudo cp TESTPRG.PRG /mnt/F2
	sudo umount /mnt/F1
	sudo umount /mnt/F2
	sudo rmdir /mnt/F1
	sudo rmdir /mnt/F2