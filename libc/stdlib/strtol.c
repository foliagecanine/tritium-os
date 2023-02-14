#include <stdio.h>
#include <limits.h>
#include <ctype.h>

static int StrCharToVal(char c)
{
    if (isdigit(c)) return c - '0';
    if (isxdigit(c))
    {
        return (toupper(c) - 'A') + 10;
    }
    return -1;
}

long int strtol(const char *str, char **endptr, int base)
{
    int         sign = 1;
    const char *cstr = str;
    while (*cstr)
    {
        if (!isspace((int)*cstr)) break;
        cstr++;
    }
    if (*cstr == '+')
    {
        cstr++;
    }
    else if (*cstr == '-')
    {
        sign = -1;
        cstr++;
    }
    const char *strbegin = cstr;
    while (StrCharToVal(*cstr) != -1 && StrCharToVal(*cstr) < base) cstr++;
    if (endptr != NULL) *endptr = (char *)cstr;
    long int multiplier = 1;
    long int out        = 0;
    while (cstr-- != strbegin)
    {
        if (out + (StrCharToVal(*cstr) * multiplier) < out) return LONG_MAX;
        out += StrCharToVal(*cstr) * multiplier;
        multiplier *= base;
    }
    out *= sign;
    return out;
}