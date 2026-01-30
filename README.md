# TritiumOS
Simple 32 bit Hobby OS  
buildable: YES :heavy_check_mark:

Original Author: foliagecanine

## Introduction
TritiumOS is an open-source operating system, successor to an
unpublished, expiremental test OS named Rogue2OS

Demo:  
![Demo GIF of TritiumOS](https://github.com/foliagecanine/tritium-os/blob/master/kernel/docs/TritiumOS.gif)  
Demo made on 07/06/20 with commit b406b609857eab945c813f9d93f506bcf30133fe

## How do you build this?
Before you start I will warn you: building a cross-compiler will take a LOT of space (~4-5GB).  
If you already have an i686-elf cross-compiler, this should not take much space at all.  
If you don't neccesarily want to build this, you can see the "What if I just want to use the ISO..." section below.

### Prerequisites
First you will need to build a compiler.  
See [https://wiki.osdev.org/Building_GCC] for info on how to do that.  
You will need an i686-elf C (GCC) compiler to build this project.  

Additionally, to compile ELF programs for TritiumOS you will need an i686-tritium compiler to build this project.  
You can build one using the [tritium-compiler](https://github.com/foliagecanine/tritium-compiler) repository.  
You can build the C library using the [libc-tritium](https://github.com/foliagecanine/libc-tritium/tree/rewrite) repository.

Then you will need QEMU, which you can get on a Debian distribution (like Ubuntu, etc.) by running this command:
`sudo apt-get install qemu`
or for other distributions
`sudo [Package Manager Install Command] qemu`

You need mtools to create the filesystem image. You can get it by running this command:
`sudo apt-get install mtools`
or for other distributions
`sudo [Package Manager Install Command] mtools`

You will also need grub-mkrescue, which is a part of the grub2 package. You can get it by running this command:
`sudo apt-get install grub2`
or for other distributions
`sudo [Package Manager Install Command] grub2`

You can alternatively build it from source, which you can find at [https://ftp.gnu.org/gnu/grub/grub-2.02.tar.xz](https://ftp.gnu.org/gnu/grub/grub-2.02.tar.xz)
(This statement is REQUIRED by the GPL, under which grub is licensed)

### Building/testing the ISO
Type `make` to create the ISO file
OR  
Type `make run` to build then immediately test it.
You can also type `./scripts/qemu.sh PARAM1 PARAM2` to add up to 2 parameters to the QEMU line (you can add more if you put them in quotes).
You can use this to add virtual hard disks to the OS.

A FAT16 formatted disk image containing programs is included (named exampledisk.img) and can be used as a hard disk as below  
`./scripts/ahci-qemu.sh exampledisk.img`

You can also have two drives by using the ahci-qemu2.sh script as below:  
`./scripts/ahci-qemu.sh exampledisk.img old/floppy2.flp`  

There are three images included that can be used for testing.  
 - exampledisk.img  : A FAT16 formatted image that contains programs to run (>512 bytes)
 - old/floppy2.flp	: A FAT12 formatted image for testing files less than 512 bytes
 - old/floppy.flp 	: A FAT12 formatted image for testing files greater than 512 bytes
 - old/testrand.img	: An image created by `dd if=/dev/urandom of=testrand.img bs=1K count=1440`, to be used for testing images with no valid filesystem.

## How do I make programs for TritiumOS?
For information on writing programs for TritiumOS, see [prgms/docs/COMPILING.md](prgms/docs/COMPILING.md)

## What if I just want to use the ISO? How can I do that?

You can use the .iso file in the root of this repository and run it in a virtual machine.
If you want to run it on real hardware (not recommended), write it to a CD/DVD, thumb drive, (even a hard drive if you're brave enough). Then have the computer boot from it.

SEE BOTTOM FOR DISCLAIMERS

## When will it be done?
Never. I will continue adding to it and editing it until I get bored or just stop for some reason.

There will be no "finished" state, but at the top of this README you will see the word "buildable:" either the word "YES" or "NO". If it's YES then you can build it without any problems. If it says "NO", then you can browse the older commits until you find one that is buildable.

## What's next (TODO)?
Here's the checklist:
- [x] Printing to terminal
- [x] GDT
- [x] Memory Management (improved)
- [x] Interrupts
- [x] Keyboard  
  --- Disk IO (ATA PIO) (not likely to be implemented)  
- [x] FAT12 Filesystem Driver
- [x] File management
- [x] Advanced Disk IO (AHCI)
- [x] Ring 3 Switching
- [x] Syscalls
- [x] Program Loading (mostly)
- [x] Shell
- [x] FAT16 Filesystem Driver
- [x] Arguments
- [x] Environment variables
- [x] FAT12/FAT16 file creation and writing
- [x] UHCI USB support  
- [x] Generic USB Driver (mostly)
- [x] Basic USB HID Driver
- [x] xHCI USB support
- [ ] Basic graphics framebuffer
- [ ] OHCI USB support  
If you want more, I will generally stick to this list: [https://wiki.osdev.org/Creating_an_Operating_System]

Programs:
- [x] Directory listing program
- [x] Program to display and edit text files
- [x] Text adventure game
- [ ] Graphics Manager
- [ ] Window Manager

## Bugs
This software is VERY buggy. There are bugs almost everywhere, but most are minor  
Impactful bugs will most likely get fixed in coming releases, while minor ones may not get fixed at all.

If you would like, you can fork this repository and fix a bug yourself.  
You can even submit a pull request. No promises about whether it's going to get merged though.

## DISCLAIMERS

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

THIS SOFTWARE MAY NOT BE TESTED ON REAL HARDWARE AND MAY, IN RARE CASES,
DAMAGE OR RENDER HARDWARE UNUSABLE. IT IS AT YOUR OWN RISK THAT YOU USE
THIS SOFTWARE. THE AUTHORS OR DISTRIBUTORS ARE NOT LIABLE FOR ANY DAMAGE
OR LOSS OF DATA THAT MAY RESULT FROM THE USE OF THIS SOFTWARE.
