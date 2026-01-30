CFLAGS=-g
CPPFLAGS=-g

PRGMS=\
shell/SHELL.SYS \
mem/MEM.PRG \
cat/CAT.PRG \
advnture/ADVNTURE.PRG \
ls/LS.PRG \
mv/MV.PRG \
gui/GUI.SYS \
edit/EDIT.PRG \
keyread/KEYREAD.PRG \
fork/FORK.PRG \
asm/ASM.PRG \
wm/WM.SYS \
hq9comp/HQ9COMP.PRG \

.PHONY: all install clean create_image $(PRGMS)

all: install

install: create_image $(PRGMS)
	mcopy -D o -i ../exampledisk.img ../kernel/myos.kernel ::/boot/; \

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
		mmd -i ../exampledisk.img bin; \
		mmd -i ../exampledisk.img bin/os; \
		mmd -i ../exampledisk.img prgms; \
		mcopy -s -D o -i ../exampledisk.img ../fs/fstemplate/* ::/; \
	fi

$(PRGMS):
	$(MAKE) -C $(dir $@) install
