# TritiumOS
Simple 32 bit Hobby OS  
buildable: YES

##Introduction
TritiumOS is an open-source operating system, successor to an unpublished, expiremental test OS named Rogue2OS

## How to build this?
First you will need to build a compiler.  
See [https://wiki.osdev.org/Building_GCC] for info on how to do that.  
You will need an i686-elf or x86_64-elf GCC compiler to build this project.

### What are the rpi-* shell scripts?
These are for building on a Raspberry Pi system. However, this is not recommended for these reasons:

1)It takes forever to build GCC on a Raspberry Pi (abbr. RPI)  
2)You ALSO have to build GRUB on the RPI which is another forever of waiting  
3)You have to make a few scripts (I guess I made them for you though)  

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
- [ ] Memory Management
- [ ] Interrupts
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
