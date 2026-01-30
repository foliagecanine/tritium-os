#include <stdio.h>

char *out;

static int sprint(const char *data, size_t length)
{
    for (size_t i = 0; i < length; i++)
        *(out++) = data[i];
    return length;
}

static size_t remain;

static int snprint(const char *data, size_t length)
{
    for (size_t i = 0; i < length && i < remain--; i++)
        *(out++) = data[i];
    return length;
}

int sprintf(char *s, const char *format, ...)
{
    va_list parameters;
    va_start(parameters, format);
    out         = s;
    int written = __print_formatted(sprint, format, parameters);
    s[written]  = 0;
    va_end(parameters);
    return written;
}

int vsprintf(char *s, const char *format, va_list arg)
{
    out         = s;
    int written = __print_formatted(sprint, format, arg);
    return written;
}

int snprintf(char *s, size_t n, const char *format, ...)
{
    va_list parameters;
    va_start(parameters, format);
    out         = s;
    remain      = n;
    size_t written = (size_t)__print_formatted(snprint, format, parameters);
    if (n > 0)
        s[(written < n) ? written : n - 1] = 0;
    va_end(parameters);
    return written;
}

int vsnprintf(char *s, size_t n, const char *format, va_list arg)
{
    out         = s;
    remain      = n;
    size_t written = (size_t)__print_formatted(snprint, format, arg);
    if (n > 0)
        s[(written < n) ? written : n - 1] = 0;
    return written;
}

int vsscanf(const char *s, const char *format, va_list parameters)
{
    (void)s;
    (void)format;
    (void)parameters;
    // Sad :(
    return 0;
}

int sscanf(const char *s, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int retval = vsscanf(s, format, args);
    va_end(args);
    return retval;
}
