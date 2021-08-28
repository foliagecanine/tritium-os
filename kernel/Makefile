DEFAULT_HOST!=../scripts/default-host.sh
HOST?=DEFAULT_HOST
HOSTARCH!=../scripts/target-triplet-to-arch.sh $(HOST)

NASM=nasm

CFLAGS?=-O3 -s -Wall -Wextra -pedantic
CPPFLAGS?=
LDFLAGS?=
LIBS?=

DESTDIR?=
PREFIX?=/usr/local
EXEC_PREFIX?=$(PREFIX)
BOOTDIR?=$(EXEC_PREFIX)/boot
INCLUDEDIR?=$(PREFIX)/include

CFLAGS:=$(CFLAGS) -ffreestanding -Wall -Wextra -msse -msse2 -ftree-vectorize
CPPFLAGS:=$(CPPFLAGS) -D__is_kernel -Iinclude
LDFLAGS:=$(LDFLAGS)
LIBS:=$(LIBS) -nostdlib -lk -lgcc

ARCHDIR=arch/$(HOSTARCH)
FSDIR=fs
USBDIR=usb
SYSDIR=sys

include $(ARCHDIR)/make.config
include $(FSDIR)/make.config
include $(USBDIR)/make.config
include $(SYSDIR)/make.config

CFLAGS:=$(CFLAGS) $(KERNEL_ARCH_CFLAGS)
CPPFLAGS:=$(CPPFLAGS) $(KERNEL_ARCH_CPPFLAGS)
LDFLAGS:=$(LDFLAGS) $(KERNEL_ARCH_LDFLAGS)
LIBS:=$(LIBS) $(KERNEL_ARCH_LIBS)

KERNEL_OBJS=\
$(KERNEL_ARCH_OBJS) \
$(KERNEL_NOARCH_OBJS) \
kernel/kernel.o \

OBJS=\
$(ARCHDIR)/sys/crti.o \
$(ARCHDIR)/crtbegin.o \
$(KERNEL_OBJS) \
$(ARCHDIR)/crtend.o \
$(ARCHDIR)/sys/crtn.o \

LINK_LIST=\
$(LDFLAGS) \
$(ARCHDIR)/sys/crti.o \
$(ARCHDIR)/crtbegin.o \
$(KERNEL_OBJS) \
$(LIBS) \
$(ARCHDIR)/crtend.o \
$(ARCHDIR)/sys/crtn.o \

.PHONY: all clean install install-headers install-kernel
.SUFFIXES: .o .c .S .asm
.SILENT: all myos.kernel install install-headers install-kernel

all: myos.kernel

myos.kernel: $(OBJS) $(ARCHDIR)/linker.ld
	@$(CC) -T $(ARCHDIR)/linker.ld -o $@ $(CFLAGS) $(LINK_LIST)
	@echo [LINK] $(ARCHDIR)/linker.ld
	@#grub-file --is-x86-multiboot myos.kernel

$(ARCHDIR)/crtbegin.o $(ARCHDIR)/crtend.o:
	@OBJ=`$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(@F)` && cp "$$OBJ" $@
	@echo [LINK] $(@F)

.c.o:
	@$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)
	@echo [CC] $<

.S.o:
	@$(CC) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)
	@echo [GAS] $<

.asm.o:
	@$(NASM) -felf32 $< -o $@
	@echo [NASM] $<

clean:
	rm -f myos.kernel
	rm -f $(OBJS) *.o */*.o */*/*.o
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d

install: install-headers install-kernel

install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/.

install-kernel: myos.kernel
	mkdir -p $(DESTDIR)$(BOOTDIR)
	cp myos.kernel $(DESTDIR)$(BOOTDIR)

-include $(OBJS:.o=.d)
