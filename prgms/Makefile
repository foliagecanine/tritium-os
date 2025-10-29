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
		if [ ! -f ../fs/disk_template.img ]; then \
			echo "Disk template not found. Creating..."; \
			./../scripts/create_disk_template.sh ../fs/disk_template.img; \
		fi; \
		\
		echo "Creating exampledisk.img..."; \
		cp ../fs/disk_template.img ../exampledisk.img; \
		mcopy -i ../exampledisk.img ../kernel/myos.kernel ::/boot/; \
		mmd -i ../exampledisk.img bin; \
		mmd -i ../exampledisk.img bin/os; \
		mmd -i ../exampledisk.img prgms; \
		mcopy -i ../exampledisk.img ../fs/fstemplate/* ::/; \
	fi

$(PRGMS):
	$(MAKE) -C $(dir $@) install
