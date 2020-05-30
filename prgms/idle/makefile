all: idle.asm
	nasm -fbin idle.asm -o IDLE.SYS

upload: IDLE.SYS
	sudo mkdir -p /mnt/F1
	sudo mkdir -p /mnt/F2
	sudo mount -o loop,rw ../../floppy.flp /mnt/F1
	sudo mount -o loop,rw ../../floppy2.flp /mnt/F2
	sudo cp IDLE.SYS /mnt/F1
	sudo cp IDLE.SYS /mnt/F2
	sudo umount /mnt/F1
	sudo umount /mnt/F2
	sudo rmdir /mnt/F1
	sudo rmdir /mnt/F2