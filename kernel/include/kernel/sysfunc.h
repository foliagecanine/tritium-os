#ifndef _KERNEL_SYSFUNC_H
#define _KERNEL_SYSFUNC_H

void terminal_putentryat(unsigned char c, uint8_t color, size_t x, size_t y);
char getchar();
unsigned int getkey();
void yield();
uint32_t exec_syscall(char *name, char **arguments, char **environment);
uint32_t getpid();
uint32_t free_pages();
uint32_t terminal_option(uint8_t command, uint8_t x, uint8_t y);
void waitpid(uint32_t pid);
uint32_t get_retval();
uint64_t get_ticks();
//uint32_t get_process_state(uint32_t pid);

#endif