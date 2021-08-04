#ifndef _ESTRING_H
#define _ESTRING_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

// estring: extensible string
// Automatically growing string

typedef struct {
	char *str;
	size_t len;
} estring;

// Initialize an estring with a length of len.
// On success returns an estring pointer
// On failure returns NULL
estring *estring_create();

// Append characters to an estring
// On success returns the estring passed in es
// On failure returns NULL
estring *estring_write(estring *es, const char *str);

// Get a pointer to the char array contained in the estring
// On success returns a char array (char pointer)
// On failure returns NULL (only happens if es is NULL)
char *estring_getstr(estring *es);

// Get the length of the data stored in the estring
size_t estring_len(estring *es);

// Append an estring to another estring
// On success returns the destination estring
// On failure returns NULL (only happens if dest or src is NULL)
estring *estring_append(estring *dest, estring *src);

// Frees an estring
void estring_delete(estring *es);

#endif