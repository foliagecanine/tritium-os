all: main.c
	gcc -m32 -Os -s -fno-pie -c -nostdlib -nostartfiles -fcf-protection=none -nodefaultlibs -fno-builtin main.c -o main.o
	ld -T linker.ld main.o -o EXEC.PRG
	
dump: EXEC.PRG
	objdump -D -b binary -mi386 EXEC.PRG
	
upload: EXEC.PRG
	sudo mkdir -p /mnt/F1
	sudo mkdir -p /mnt/F2
	sudo mount -o loop,rw ../../floppy.flp /mnt/F1
	sudo mount -o loop,rw ../../floppy2.flp /mnt/F2
	sudo cp EXEC.PRG /mnt/F1/prgms
	sudo cp EXEC.PRG /mnt/F2/prgms
	sudo umount /mnt/F1
	sudo umount /mnt/F2
	sudo rmdir /mnt/F1
	sudo rmdir /mnt/F2

clean:
	rm main.o
	rm EXEC.PRG
