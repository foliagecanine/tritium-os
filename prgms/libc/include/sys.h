#ifndef _SYS_H
#define _SYS_H

static inline void syscall(unsigned int syscall_num);
void writestring(char *string);
uint32_t exec(char *name);
uint32_t exec_args(char *name, char **arguments, char **environment);
void yield();
uint32_t waitpid(uint32_t pid);
uint32_t getpid();

#endif
