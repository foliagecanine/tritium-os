# Compiling Programs

Welcome to COMPILING.MD. Here you will find how to compile programs for TritiumOS.

## Compiling
### Requirements
For assembly programs
 - NASM, GAS, or other assembler for x86  
 
For C programs
 - GCC x86 compiler (May work with other compilers such as clang, but untested)
 - GNU ld (or equivelant, untested)
 - GNU Make (recommended)
 
### Compiling Assembly
For assembly programs, simply write the code as you would.
Start with (NASM):  
```NASM
org 0x100000
bits 32
```  
Compile it with (NASM): 
```bash
nasm -fbin input.asm -o OUTPUT.PRG
```
  
### Compiling C
For C programs, it's a little more complicated.
It is recommended to use a program like GNU Make for automation of the compilation.  

When linking, you MUST LINK *libc_directory*/sys/sys.o **FIRST**! This provides the initialization code to launch the "main" function.  
Here's an example:  
```bash
ld -T linker.ld $(LIBCDIR)/sys/sys.o main.o -o OUTPUT.PRG
```

Alternatively, you can use one of the following methods:  

In the main c file, make sure you put code such as
```C
extern char **envp;
extern uint32_t envc;
asm ("push %ecx;\
		push %eax;\
		mov %ebx,(envc);\
		mov %edx,(envp);\
		call main;\
		mov %eax,%ebx;\
		mov $2,%eax;\
		int $0x80;\
		jmp .");
```
		
This will save all arguments and environment variables then launch your code.  
When it is done, it will return the return value from the main function. This happens even if the "main" function is type void.  

At a minimum, you must have

```C
asm("jmp main");
```

The code doesn't know where to start, so by adding this you direct it to launch the "main" function.
Also remember to exit your program with syscall 2. Otherwise your program will experience an error when it returns from the main function.

Compile with the following flags (GCC):  
```bash
gcc -MD -c input.c -o output.o -std=gnu11 -m32 -Os -s -fno-pie -ffreestanding -nostartfiles
```

Note: The -m32 flag is only required when using a 64 bit compiler

This effectively disables all of gcc's built in libraries and files and prepares it for linking.

Link the program with the following flags (LD):  
```bash
ld -T linker.ld $(LIBCDIR)/sys/sys.o obj1.o obj2.o -o OUTPUT.PRG
```

or if you aren't using the startfile

```bash
ld -T linker.ld obj1.o obj2.o -o OUTPUT.PRG
```

The linker.ld file is provided in the prgms/docs folder.

## Inner Workings
### Program Memory Layout
Virtual Memory Map (for programs):

|      Memory       |      Description      |
| ----------------- | --------------------- |
| 0x100000-0x500000 | Program code and data |
| 0xF00000-0xF04000 | Stack                 |
| 0xF04000-0xF05000 | Arguments             |
| 0xF05000-0xF06000 | Environment variables |

### Variables from the Kernel
The kernel provides four variables in the following registers  
eax: Number of arguments to the main function (argc)  
ebx: Number of environment variables, used by env.c in libc  
ecx: Pointer to char\*\* of arguments (argv) (Should always be 0xF04000)  
edx: Pointer to char\*\* of environment variables (Should always be 0xF05000)  

### Software Interrupts

There are several software interrupt functions available to the user process.  
They can be called by putting the function number in eax and calling `int 0x80` (or "int $0x80" in GAS)

```C
int 0x80, eax=0: terminal writestring  
Arguments:  
ebx = (char *)string : String to write

This function simply prints a C string (terminated with 0/NULL) to the terminal.
```

```C
int 0x80, eax=1: exec_syscall
Arguments:
ebx = char  *name     : Program full path
ecx = char **args     : Arguments, terminated by NULL
edx = char **env      : Environment variables, terminated by NULL
Return:
eax = uint32_t pid    : PID of program started. 0 if failed.

This function launches a file as a program. Arguments and environment variables are supplied by callee.
```

```C
int 0x80, eax=2: exit_program  
Arguments:  
ebx = uint32_t status : Exit status

This function exits the program properly. Exit status is given to any programs using waitpid.
```

```C
int 0x80, eax=3: terminal_putentryat  
Arguments:  
bl  = uint8_t c       : Character to print
cl  = uint8_t color   : Color of character (foreground and background)
edx = size_t x        : X location on screen. Can be 0-79
esi = size_t y        : Y location on screen. Can be 0-24

This function puts a character at a specific point on the screen
```

```C
int 0x80, eax=4: getchar  
Return:
al  = char c          : Character typed on keyboard. NULL if there is none.

This function gets a character typed on the keyboard
```

```C
int 0x80, eax=5: get_kbddata  
Return:
eax  = uint32_t k      : Keyboard packet. See below.

Keyboard packet:

| Bits |   Description    |
|-------------------------|
| 0:7  | Kbd scancode     |
| 8:15 | Character        |
| 16   | Ctrl Key Status  |
| 17   | Shift Status     |
| 18   | Alt Key Status   |
| 19   | Numlock Status   |
| 20   | Scroll Lock Stat |
| 21   | Capslock Status  |
|22:31 | Unused           |

This function gets a detailed packet about a key typed on the keyboard, as well as the status of control keys (Alt, Shift, etc.)
```

```C
int 0x80, eax=6: yield

Prematurely give control to another program in the queue. Useful when waiting for time-sensitive things like delays or keyboard input.
```

```C
int 0x80, eax=7: getpid
Return:
eax = uint32_t pid    : PID of current program

Get PID of current program
```

```C
int 0x80, eax=8: free_pages
Return:
eax = uint32_t pages  : Number of memory pages (4KiB) free in the system

Get amount of free memory in the system
```

```C
int 0x80, eax=9, bl=0: terminal_option.setcursor
Arguments:
cl = uint8_t x        : X position of cursor
dl = uint8_t y        : Y position of cursor

Set the cursor position to x,y
```

```C
int 0x80, eax=9, bl=1: terminal_option.getcursor
Return:
eax = uint32_t cursor : Cursor position in format (0x0000<<16) | (x<<8) | (y)

Get the cursor position
```

```C
int 0x80, eax=9, bl=2: terminal_option.scroll

Scroll the terminal one line
```

```C
int 0x80, eax=10: waitpid
Arguments:
ebx = uint32_t pid    : PID to wait for

Wait for PID to exit
```

```C
int 0x80, eax=11: get_retval
Return:
eax = uint32_t retval : Program's return value

Get return value of program. Must be used after waitpid.
```

```C
int 0x80, eax=12: fopen
Arguments:
ebx = FILE *f		  : Return value pointer
ecx = char *filename  : Full filename of file to open
edx = char *mode	  : Mode; currently is only "r" or "w"
Return:
FILE f will be the resulting file. See kernel/include/fs/fs.h for information on the file structure.

Open a file. 
```

```C
int 0x80, eax=13: fread
Arguments:
ebx = FILE *f		  : Pointer to FILE structure
ecx = char *buf  	  : Buffer to read into
edx = uint32_t starth : High 32 bits of the start address in the file.
esi = uint32_t startl : Low 32 bits of the start address in the file.
edi = uint32_t lenl	  : Length of read. Can only read a buffer up to 4 GiB at a time.
Return:
al = return value. See kernel/arch/i386/file.c for details on return values.

Read bytes from a file. 
```

```C
int 0x80, eax=14: fwrite
Arguments:
ebx = FILE *f		  : Pointer to FILE structure
ecx = char *buf  	  : Buffer to write from
edx = uint32_t starth : High 32 bits of the start address in the file.
esi = uint32_t startl : Low 32 bits of the start address in the file.
edi = uint32_t lenl	  : Length of write. Can only write a buffer up to 4 GiB at a time.
Return:
al = return value. See kernel/arch/i386/file.c for details on return values.

Write bytes from a file. 
```

```C
int 0x80, eax=15: fcreate
Arguments:
ebx = char *filename  : Full name of file to create.
ecx = FILE *o		  : Output file pointer
Return:
FILE o will be the resulting file. See kernel/include/fs/fs.h for information on the file structure.

Create a new file.
```

```C
int 0x80, eax=16: fdelete
Arguments:
ebx = char *filename  : Full name of file to delete.
Return:
al = return value. See kernel/arch/i386/file.c for details on return values.

"Recycle" a file. Deletes it, but allows it to be recovered.
```

```C
int 0x80, eax=17: ferase
Arguments:
ebx = char *filename  : Full name of file to erase.
Return:
al = return value. See kernel/arch/i386/file.c for details on return values.

Erase a file. Deletes a file as well as its reserved space, freeing up space to be used. This is not secure, however, as the data will still be on the hard drive.
```

```C
int 0x80, eax=18: readdir
Arguments:
ebx = FILE *f		  : File pointer to directory to read.
ecx = FILE *o		  : Output pointer of file read
edx = char *buf		  : Output buffer of filename. Should be 256 characters long.
esi = uint32_t n	  : Entry number to read.
Return:
buf will contain the name of the file.
FILE o will be the resulting file. See kernel/include/fs/fs.h for information on the file structure.

Read a file from a directory entry. 
```

```C
int 0x80, eax=23: debug_break

Does nothing. Gives an accessible place to debug programs when using GDB + QEMU.
```

```C
int 0x80, eax=24: get_ticks (untested)
Return:
eax = return value of ticks (low 32 bits).
edx = return value of ticks (high 32 bits)?

```

```C
int 0x80, eax=25: fork
Return:
eax = pid of new process, or 0 for child

```