#ifndef _DECODE_H
#define _DECODE_H

#include <stdio.h>
#include <string.h>
#include "asmtypes.h"
#include "estring.h"
#include "x86ops.h"

int encode_ops(estring *in, estring *out);
int encode_labels(estring *in, estring *out);
int encode_bin(estring *in, estring *out);

#endif