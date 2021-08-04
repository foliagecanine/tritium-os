#include "estring.h"
#include <stdio.h>

estring *estring_create() {
	estring *es = malloc(sizeof(estring));
	if (!es)
		return NULL;
	es->str = malloc(1);
	es->len = 1;
	*(es->str) = 0;
	return es;
}

char *estring_getstr(estring *es) {
	if (!es)
		return NULL;
	return es->str;
}

size_t estring_len(estring *es) {
	if (!es)
		return 0;
	return es->len-1;
}

estring *estring_write(estring *es, const char *str) {
	if (!es)
		return NULL;
	es->str = realloc(es->str, es->len+strlen(str));
	memcpy(es->str+(es->len-1), str, strlen(str));
	es->len += strlen(str);
	*(es->str+(es->len-1)) = 0;
	return es;
}

estring *estring_append(estring *dest, estring *src) {
	if (!dest)
		return NULL;
	if (!src)
		return NULL;
	dest->str = realloc(dest->str, dest->len+src->len-1);
	memcpy(dest->str+(dest->len-1), src->str, src->len-1);
	dest->len += src->len-1;
	*(dest->str+(dest->len-1)) = 0;
	return dest;
}

void estring_delete(estring *es) {
	if (es)
		free(es);
}