#ifndef _DECODE_H
#define _DECODE_H

#include <stdio.h>
#include <string.h>
#include "asmtypes.h"
#include "estring.h"
#include "x86ops.h"

/**
 * Encodes assembly instructions from the input estring into machine code in the output estring.
 * 
 * @param in Pointer to the input estring containing assembly instructions.
 * @param out Pointer to the output estring where encoded machine code will be written.
 */
int encode_ops(estring *in, estring *out);

/**
 * Encodes labels from the input estring into the output estring.
 * 
 * @param in Pointer to the input estring containing labels.
 * @param out Pointer to the output estring where encoded labels will be written.
 */
int encode_labels(estring *in, estring *out);

/**
 * Encodes binary data directives from the input estring into the output estring.
 * 
 * @param in Pointer to the input estring containing binary data directives.
 * @param out Pointer to the output estring where encoded binary data will be written.
 */
int encode_bin(estring *in, estring *out);

#endif