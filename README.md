# TritiumOS
Simple 32 bit Hobby OS  
buildable: YES :heavy_check_mark:

Original Author: foliagecanine

## Introduction
TritiumOS is an open-source operating system, successor to an 
unpublished, expiremental test OS named Rogue2OS

Demo:
![Demo GIF of TririumOS](https://github.com/foliagecanine/tritium-os/master/kernel/docs/TritiumOS.gif)  
Demo made on 07/06/20 with commit b406b609857eab945c813f9d93f506bcf30133fe

## How do you build this?
Before you start I will warn you: this will take a LOT of space (~4-5GB)  
If you don't neccesarily want to build this, you can see the "What if I just want to use the ISO..." section below.

### Prerequisites
First you will need to build a compiler.  
See [https://wiki.osdev.org/Building_GCC] for info on how to do that.  
You will need an i686-elf or x86_64-elf GCC compiler to build this project.

Then you will need QEMU, which you can get on a Debian distribution (like Ubuntu, etc.) by running this command:
`sudo apt-get install qemu`
or for other distributions
`sudo [Package Manager Install Command] qemu`

You will also need grub-mkrescue, which is a part of the grub2 package. You can get it by running this command:
`sudo apt-get install grub2`
or for other distributions
`sudo [Package Manager Install Command] grub2`

You can alternatively build it from source, which you can find at [ftp://ftp.gnu.org/gnu/grub/grub-2.02.tar.xz](https://bit.ly/2ZNmsQa)* 
(This statement is REQUIRED by the GPL, under which grub is licensed)

\*If you hover on the link, it is a bit.ly link. This is because github does not properly hyperlink ftp:// links. If you are paranoid about bit.ly links or something, you can just copy the address into your url bar.

### Building/testing the iso
Then type `./iso.sh` to create the iso file 
OR  
Type `./qemu.sh` to build then immediately test it.
You can also type `./qemu.sh PARAM1 PARAM2` to add up to 2 parameters to the qemu line (you can add more if you put them in quotes).
You can use this to add virtual hard disks to the OS.

A FAT16 formatted "floppy" is included (named floppy.flp) and can be used as a hard disk as below  
`./ahci-qemu.sh floppy.flp "-boot d"` 
Or a floppy disk (no controller implemented yet) like so  
`./qemu.sh "-fda floppy.flp" "-boot d"`

You can also have two drives by using the ahci-qemu2.sh script as below:  
`./ahci-qemu.sh old/floppy2.flp floppy.flp "-boot d"`  

There are three images included that can be used for testing.  
 - floppy.flp : A FAT16 formatted image that contains programs to run (>512 bytes)
 - old/floppy2.flp	: A FAT12 formatted image for testing files less than 512 bytes
 - old/floppy.flp 	: A FAT12 formatted image for testing files greater than 512 bytes
 - old/testrand.img	: An image created by `dd if=/dev/urandom of=testrand.img bs=1K count=1440`, to be used for testing images with no valid filesystem.

### How about that add-o-file.sh script?
That is an easy way of editing the make.config file.  
You can use it to add a .o file to the list of files to be included in the build.

To use it type:  
`./add-o-file.sh [filename]`  
The filename is the name of the c/asm/S file to include without the extension. For example, type:  
`./add-o-file.sh mynewfile`  
To add mynewfile.c (or mynewfile.S or mynewfile.asm) to the make.config  
Note there is no extension in the filename.

## How do I make programs for TritiumOS?
For information on writing programs for TritiumOS, see [prgms/docs/COMPILING.md](prgms/docs/COMPILING.md)

## What if I just want to use the ISO? How can I do that?

You can use the .iso file in the root of this repository and run it in a virtual machine.
If you want to run it on real hardware (not recommended), burn it to a disk, thumb drive, (even a hard drive if you're brave enough). Then have the computer boot from it.

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
- [ ] Shell (almost there)
- [x] FAT16 Filesystem Driver
- [x] Arguments
- [x] Environment variables
- [ ] FAT12/FAT16 file creation and writing
If you want more, I will generally stick to this list: [https://wiki.osdev.org/Creating_an_Operating_System]

Programs:
- [x] Directory listing program
- [x] Program to display text files
- [ ] Program to edit text files
- [x] Text adventure game

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
