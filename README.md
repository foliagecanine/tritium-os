# TritiumOS (Development Branch)
Simple 32 bit Hobby OS  
buildable: YES :heavy_check_mark:

Original Author: foliagecanine

## Introduction
TritiumOS is an open-source operating system, successor to an 
unpublished, expiremental test OS named Rogue2OS

This branch is the Development branch of TritiumOS. For the main branch, select "master" from the list of branches.  
This branch has effectively rebuilt TritiumOS from the ground up by combining original TritiumOS code with new code. Several improvements have been made in this branch. However, several features (i.e disk IO, file systems) have been removed. These features will eventually come back.

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

A FAT12 formatted "floppy" is included (named floppy2.flp) and can be used as a hard disk as below  
`./ahci-qemu.sh floppy2.flp "-boot d"`  
Or a floppy disk (no controller implemented yet) like so  
`./qemu.sh "-fda floppy2.flp" "-boot d"`

You can also have two drives by using the ahci-qemu2.sh script as below:  
`./ahci-qemu.sh floppy2.flp floppy.flp "-boot d"

There are three images included that can be used for testing.  
 - floppy2.flp	: A FAT12 formatted image for testing files less than 512 bytes
 - floppy.flp 	: A FAT12 formatted image for testing files greater than 512 bytes
 - testrand.img	: An image created by `dd if=/dev/urandom of=testrand.img bs=1K count=1440`, to be used for testing images with no valid filesystem.

### What are the rpi-* shell scripts?
These are for building on a Raspberry Pi system. However, this is not recommended for these reasons:

1)It takes forever to build GCC on a Raspberry Pi (abbr. RPI)  
2)You ALSO have to build GRUB on the RPI which is another forever of waiting  
3)You have to make a few scripts (I guess I made them for you though) 

The only difference between the normal scripts and rpi-\* scripts is that when calling grub-mkrescue it instead calls i686-grub-mkrescue

### How about that add-o-file.sh script?
That is an easy way of editing the make.config file.  
You can use it to add a .o file to the list of files to be included in the build.

To use it type:  
`./add-o-file.sh [filename]`  
The filename is the name of the c/asm/S file to include without the extension. For example, type:  
`./add-o-file.sh mynewfile`  
To add mynewfile.c (or mynewfile.S or mynewfile.asm) to the make.config  
Note there is no extension in the filename.

## When will it be done?
Never. I will continue adding to it and editing it until I get bored or just stop for some reason.

There will be no "finished" state, but at the top of this README you will see the word "buildable:" either the word "YES" or "NO". If it's YES then you can build it without any problems. If it says "NO", then you can browse the older commits until you find one that is buildable.

## What if I just want to use the ISO? How can I do that?

You can use the .iso file in the root of this repository and burn it to a disk, thumb drive, (even a hard drive if you're brave enough). Then have the computer boot from it.

SEE BOTTOM FOR DISCLAIMERS

## What's next (TODO)?
Here's the checklist:
- [x] Printing to terminal
- [x] GDT
- [x] Memory Management (improved)
- [ ] Interrupts (partial)
- [x] Keyboard
  --- Disk IO (ATA PIO) (not likely to be implemented)
- [x] FAT12 Filesystem Driver
- [x] File management
- [ ] Advanced Disk IO (AHCI) (partial)
If you want more, I will generally stick to this list: [https://wiki.osdev.org/Creating_an_Operating_System]

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
