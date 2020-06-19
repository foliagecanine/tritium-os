GCC=gcc

all: idle.asm
	nasm -fbin idle.asm -o IDLE.SYS

upload: IDLE.SYS
	sudo mkdir -p /mnt/F1
	sudo mount -o loop,rw ../../floppy.flp /mnt/F1
	sudo cp IDLE.SYS /mnt/F1/bin
	sudo umount /mnt/F1
	sudo rmdir /mnt/F1

install: IDLE.SYS
	sudo cp IDLE.SYS /mnt/F1/bin
