CFLAGS=-g
CPPFLAGS=-g

PRGMS=\
shell/SHELL.SYS \
mem/MEM.PRG \
cat/CAT.PRG \
advnture/ADVNTURE.PRG \
ls/LS.PRG \
gui/GUI.SYS \
edit/EDIT.PRG \
keyread/KEYREAD.PRG \
fork/FORK.PRG \
asm/ASM.PRG \
wm/WM.SYS \
ipctest/IPCTEST.PRG \
hq9comp/HQ9COMP.PRG \

.PHONY: all install clean create_image $(PRGMS)

all: $(PRGMS)

install: create_image $(PRGMS)

clean:
	rm $(PRGMS)
	rm ../exampledisk.img
	rm */*.o

create_image:
	@if [ ! -f ../exampledisk.img ]; then \
		echo "Creating exampledisk.img..."; \
		dd if=/dev/zero of=../exampledisk.img bs=1M count=1; \
		mformat -i ../exampledisk.img -T 65536 -v "TRITIUM OS " ::; \
		mmd -i ../exampledisk.img bin; \
		mmd -i ../exampledisk.img bin/os; \
		mmd -i ../exampledisk.img prgms; \
		mcopy -i ../exampledisk.img ../fstemplate/* ::/; \
	fi

$(PRGMS):
	$(MAKE) -C $(dir $@) install
