#ifndef _ESTRING_H
#define _ESTRING_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

// estring: extensible string
// Automatically growing string

typedef struct
{
    char * str;
    size_t len;
} estring;

/**
 * Create an estring.
 * 
 * @returns On success returns an estring pointer. On failure returns NULL.
 */
estring *estring_create();

/**
 * Append a single character to an estring
 * 
 * @param es Pointer to the estring to append to
 * @param c Character to append
 */
estring *estring_putchar(estring *es, const char c);

/**
 * Append characters to an estring
 * 
 * @param es Pointer to the estring to append to
 * @param str String to append
 * @returns On success returns the estring passed in es. On failure returns NULL.
 */
estring *estring_write(estring *es, const char *str);

/**
 * Get the C-style string from an estring
 * 
 * @param es Pointer to the estring
 * @returns On success returns a char array (char pointer). On failure returns NULL (only happens if es is NULL).
 */
char *estring_getstr(estring *es);

/**
 *  Get the length of the data stored in the estring
 */
size_t estring_len(estring *es);

/**
 * Append an estring to another estring
 * 
 * @param dest Pointer to the destination estring
 * @param src Pointer to the source estring
 * @returns On success returns the destination estring. On failure returns NULL (only happens if dest or src is NULL)
 */
estring *estring_append(estring *dest, estring *src);

/**
 * Delete an estring and free its memory
 * 
 * @param es Pointer to the estring to delete
 */
void estring_delete(estring *es);

#endif