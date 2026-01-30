#ifndef _ELF_H
#define _ELF_H

#include <stdio.h>

/**
 * Encodes the given binary data into a minimal ELF executable.
 * 
 * @param in Pointer to the binary data to be included in the ELF.
 * @param len_in Length of the binary data in bytes.
 * @param len_out Pointer to a size_t variable where the length of the resulting ELF data will be stored.
 * @return Pointer to the newly allocated ELF data. The caller is responsible for freeing this memory.
 */
void *encode_elf(void *in, size_t len_in, size_t *len_out);

#endif
