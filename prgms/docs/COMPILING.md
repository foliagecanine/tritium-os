# Compiling Programs

Welcome to COMPILING.MD. Here you will find how to compile programs for TritiumOS.

## Compiling
### Requirements
For assembly programs
 - NASM, GAS, or other assembler for x86  
 - i686-tritium-ld (or equivelant, untested)

For C programs
 - i686-tritium-gcc compiler (May work with other compilers such as clang, but untested)
 - i686-tritium-ld (or equivelant, untested)
 - GNU Make (recommended)

### Compiling Assembly
For assembly programs, simply write the code as you would.
Compile it with (NASM):
```bash
nasm -felf32 input.asm
i686-tritium-ld input.o -o OUTPUT.PRG
```

### Compiling C
It is recommended to use a program like GNU Make for automation of the compilation.  

Compiling a single C file into an executable (GCC):  
```bash
i686-tritium-gcc main.c -o OUTPUT.PRG
```

Linking multiple C files into an executable (GCC and LD):  
```bash
i686-tritium-gcc -c file1.c
i686-tritium-gcc -c file2.c
i686-tritium-ld file1.o file2.o -o OUTPUT.PRG
```

## Inner Workings
### Variables from the Kernel
The kernel provides four variables in the following registers  
eax: Number of arguments to the main function (argc)  
ebx: Number of environment variables, used by env.c in libc  
ecx: Pointer to char\*\* of arguments (argv)  
edx: Pointer to char\*\* of environment variables  

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

Get the number of 1ms ticks since startup
```

```C
int 0x80, eax=25: fork
Return:
eax = pid of new process, or 0 for child

Fork the current process into a new process with its own PID
```

```C
int 0x80, eax=26: map_mem
Arguments:
ebx = void * address	: Address to add a page (4KiB) of memory at
Return:
eax = address of memory, 0 if error

Map a page of memory for the process to use
```
