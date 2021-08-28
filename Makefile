.PHONY: all clean pclean run myos.iso

myos.iso: 
	./scripts/iso.sh

run: 
	./scripts/ahci-qemu.sh exampledisk.img

all: myos.iso

clean:
	./scripts/clean.sh

pclean:
	./scripts/pclean.sh
