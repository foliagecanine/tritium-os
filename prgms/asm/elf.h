#ifndef _ELF_H
#define _ELF_H

#include <stdio.h>

void *encode_elf(void *in, size_t len_in, size_t *len_out);

#endif
