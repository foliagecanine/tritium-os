.PHONY: all clean run myos.iso

myos.iso: 
	./scripts/iso.sh

run: myos.iso
	./scripts/ahci-qemu.sh exampledisk.img

all: myos.iso

clean:
	./scripts/clean.sh
