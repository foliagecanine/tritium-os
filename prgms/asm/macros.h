#ifndef _MACROS_H
#define _MACROS_H

#include <stdio.h>
#include "estring.h"

/**
 * Encodes macros from the input estring into the output estring.
 * 
 * @param in Pointer to the input estring containing macros.
 * @param len_out Pointer to a size_t variable where the length of the resulting encoded data will be stored.
 * @return Pointer to the newly allocated encoded data. The caller is responsible for freeing this memory.
 */
void *encode_macros(estring *in, size_t *len_out);

#endif
