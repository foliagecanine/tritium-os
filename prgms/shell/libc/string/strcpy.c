#include <string.h>

#pragma GCC diagnostic ignored "-Wparentheses"
char *strcpy(char *dest, const char *src)
{
   char *ptr = dest;
   while(*dest++ = *src++);
   return ptr;
}