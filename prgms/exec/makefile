GCC=i686-elf-gcc

all: EXEC.PRG

EXEC.PRG: main.c
	gcc -m32 -Os -s -fno-pie -ffreestanding-fno-stack-protector -c main.c -o main.o
	ld -T linker.ld main.o -o EXEC.PRG
	
dump: EXEC.PRG
	objdump -D -b binary -mi386 EXEC.PRG
	
upload: EXEC.PRG
	sudo mkdir -p /mnt/F1
	sudo mount -o loop,rw ../../floppy.flp /mnt/F1
	sudo cp EXEC.PRG /mnt/F1/prgms
	sudo umount /mnt/F1
	sudo rmdir /mnt/F1

clean:
	rm main.o
	rm EXEC.PRG

install: EXEC.PRG
	sudo cp EXEC.PRG /mnt/F1/prgms
