#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

typedef enum __printf_flags
{
    PRINTF_FLAG_NONE      = (0),
    PRINTF_FLAG_LJUSTIFY  = (1 << 0),
    PRINTF_FLAG_FORCESIGN = (1 << 1),
    PRINTF_FLAG_SPACE     = (1 << 2),
    PRINTF_FLAG_PRECEDE   = (1 << 3),
    PRINTF_FLAG_LEFTPAD   = (1 << 4)
} __printf_flags;

/**
 * Gets the printf flags
 *
 * @param[in, out] format a pointer to the current format pointer
 *
 * @returns a printf flag that matches a PRINTF_FLAG_[*] constant
 */
static __printf_flags PrintfGetFlags(const char **format)
{
    __printf_flags flags = PRINTF_FLAG_NONE;

    if (**format == '-')
    {
        flags |= PRINTF_FLAG_LJUSTIFY;
        (*format)++;
    }

    if (**format == '+')
    {
        flags |= PRINTF_FLAG_FORCESIGN;
        (*format)++;
    }

    if (**format == ' ')
    {
        flags |= PRINTF_FLAG_SPACE;
        (*format)++;
    }

    if (**format == '#')
    {
        flags |= PRINTF_FLAG_PRECEDE;
        (*format)++;
    }

    if (**format == '0')
    {
        flags |= PRINTF_FLAG_LEFTPAD;
    }

    return flags;
}

typedef enum __printf_width
{
    PRINTF_WIDTH_NONE = -(1 << 0)
} __printf_width;

/**
 * Gets the printf width
 *
 * @param[in, out] format a pointer to the current format pointer
 *
 * @returns the width specified, or PRINTF_WIDTH_NONE if none specified
 */
static __printf_width PrintfGetWidth(const char **format)
{
    if (!isdigit(**format))
    {
        return PRINTF_WIDTH_NONE;
    }

    char *   endptr;
    long int num = strtol(*format, &endptr, 10);
    *format      = endptr;
    return (__printf_width)(num);
}

typedef enum __printf_precision
{
    PRINTF_PRECISION_NONE = -(1 << 0)
} __printf_precision;

/**
 * Gets the printf precision
 *
 * @param[in, out] format a pointer to the current format pointer
 *
 * @returns the precision specified, or PRINTF_PRECISION_NONE if none specified
 */
static __printf_precision PrintfGetPrecision(const char **format)
{
    if (**format != '.')
    {
        return PRINTF_PRECISION_NONE;
    }

    (*format)++;

    char *   endptr;
    long int num = strtol(*format, &endptr, 10);
    if (endptr != *format)
    {
        *format = endptr;
        return (__printf_precision)(num);
    }

    return (__printf_precision)(0);
}

typedef enum __printf_length
{
    PRINTF_LENGTH_NONE      = (0),
    PRINTF_LENGTH_CHAR      = (1 << 0),
    PRINTF_LENGTH_SHORT     = (1 << 1),
    PRINTF_LENGTH_LONG      = (1 << 2),
    PRINTF_LENGTH_LONGLONG  = (1 << 3),
    PRINTF_LENGTH_INTMAX_T  = (1 << 4),
    PRINTF_LENGTH_SIZE_T    = (1 << 5),
    PRINTF_LENGTH_PTRDIFF_T = (1 << 6),
    PRINTF_LENGTH_LONGDBL   = (1 << 7)
} __printf_length;

/**
 * Gets the printf length modifier
 *
 * @param[in, out] format a pointer to the current format pointer
 *
 * @returns a length modifier constant that matches PRINTF_LENGTH_[*]
 */
__printf_length PrintfGetLength(const char **format)
{
    if (**format == 'h')
    {
        (*format)++;
        if (**format == 'h')
        {
            (*format)++;
            return PRINTF_LENGTH_CHAR;
        }
        return PRINTF_LENGTH_SHORT;
    }

    if (**format == 'l')
    {
        (*format)++;
        if (**format == 'l')
        {
            (*format)++;
            return PRINTF_LENGTH_LONGLONG;
        }
        return PRINTF_LENGTH_LONG;
    }

    if (**format == 'j')
    {
        (*format)++;
        return PRINTF_LENGTH_INTMAX_T;
    }

    if (**format == 'z')
    {
        (*format)++;
        return PRINTF_LENGTH_SIZE_T;
    }

    if (**format == 't')
    {
        (*format)++;
        return PRINTF_LENGTH_PTRDIFF_T;
    }

    if (**format == 't')
    {
        (*format)++;
        return PRINTF_LENGTH_LONGDBL;
    }

    return PRINTF_LENGTH_NONE;
}

static const char __printfFormatAllowed[] = {'d', 'i', 'u', 'o', 'x', 'X', 'f', 'F', 'e', 'E',
                                             'g', 'G', 'a', 'A', 'c', 's', 'p', 'n', '%', '\0'};

typedef enum __printf_format
{
    PRINTF_FORMAT_DECIMAL    = 'd',
    PRINTF_FORMAT_SIGNED     = 'i',
    PRINTF_FORMAT_UNSIGNED   = 'u',
    PRINTF_FORMAT_OCTAL      = 'o',
    PRINTF_FORMAT_UHEX       = 'x',
    PRINTF_FORMAT_UHEXUP     = 'X',
    PRINTF_FORMAT_FLOAT      = 'f',
    PRINTF_FORMAT_FLOATUP    = 'F',
    PRINTF_FORMAT_SCI        = 'e',
    PRINTF_FORMAT_SCIUP      = 'E',
    PRINTF_FORMAT_SHORT      = 'g',
    PRINTF_FORMAT_SHORTUP    = 'G',
    PRINTF_FORMAT_HEXFLOAT   = 'a',
    PRINTF_FORMAT_HEXFLOATUP = 'A',
    PRINTF_FORMAT_CHAR       = 'c',
    PRINTF_FORMAT_STRING     = 's',
    PRINTF_FORMAT_POINTER    = 'p',
    PRINTF_FORMAT_STORE      = 'n',
    PRINTF_FORMAT_PERCENT    = '%',
    PRINTF_FORMAT_ERROR      = '\0'
} __printf_format;

/**
 * Gets the printf format mode
 *
 * @param[in, out] format a pointer to the current format pointer
 *
 * @returns a printf format mode that matches a PRINTF_FORMAT_[*] constant
 */
static __printf_format PrintfGetFormat(const char **format)
{
    for (unsigned int i = 0; i < sizeof(__printfFormatAllowed) / sizeof(char); i++)
    {
        if (__printfFormatAllowed[i] == **format)
        {
            __printf_format formatChar = (__printf_format)(**format);
            (*format)++;
            return formatChar;
        }
    }

    return PRINTF_FORMAT_ERROR;
}

/**
 * Prints a formatted string using printfn as the function
 *
 * @param[in] printfn the print function
 * @param[in] format the format to print
 * @param[in] parameters the format replacements
 */
int __print_formatted(int (*printfn)(const char *, size_t), const char *format, va_list parameters)
{
    int written = 0;

    while (*format != '\0')
    {
        if (*format != '%')
        {
            written += printfn(format, 1);
            format++;
            continue;
        }

        format++;

        __printf_flags     printFlags     = PrintfGetFlags(&format);
        __printf_width     printWidth     = PrintfGetWidth(&format);
        __printf_precision printPrecision = PrintfGetPrecision(&format);
        __printf_length    printLength    = PrintfGetLength(&format);
        __printf_format    printFormat    = PrintfGetFormat(&format);

        if (printFormat == PRINTF_FORMAT_ERROR)
        {
            continue;
        }

        size_t width     = (size_t)printWidth;
        size_t precision = (size_t)printPrecision;

        if (printFormat == PRINTF_FORMAT_PERCENT)
        {
            written += printfn("%", 1);
        }
        else if (printFormat == PRINTF_FORMAT_CHAR)
        {
            if (printLength & PRINTF_LENGTH_LONG)
            {
                wchar_t *printReadInChar = va_arg(parameters, void *);
                written += printfn((char *)printReadInChar, 1);
            }
            else
            {
                unsigned char printReadinChar = va_arg(parameters, int);
                written += printfn((char *)&printReadinChar, 1);
            }
        }
        else if (printFormat == PRINTF_FORMAT_STRING)
        {
            char *read = (char *)va_arg(parameters, void *);
            if (precision == (size_t)PRINTF_PRECISION_NONE)
            {
                written += printfn(read, strlen(read));
            }
            else
            {
                // TODO SPEC: Wide chars may not work correctly
                int haveWritten = 0;
                while (*read)
                {
                    if ((size_t)haveWritten == precision)
                    {
                        break;
                    }
                    written += printfn(read, 1);
                    haveWritten++;
                    if (printLength & PRINTF_LENGTH_LONG)
                    {
                        haveWritten++;
                        read++;
                    }
                    read++;
                }
            }
        }
        else if (printFormat == PRINTF_FORMAT_DECIMAL || printFormat == PRINTF_FORMAT_SIGNED)
        {
            long long value = 0;
            if (printLength == PRINTF_LENGTH_CHAR || printLength == PRINTF_LENGTH_SHORT ||
                printLength == PRINTF_LENGTH_NONE)
            {
                value = (long long)va_arg(parameters, int);
            }
            else if (printLength == PRINTF_LENGTH_LONG)
            {
                value = (long long)va_arg(parameters, long);
            }
            else if (printLength == PRINTF_LENGTH_LONGLONG)
            {
                value = (long long int)va_arg(parameters, long long);
            }
            else if (printLength == PRINTF_LENGTH_INTMAX_T)
            {
                value = (long long)va_arg(parameters, intmax_t);
            }
            else if (printLength == PRINTF_LENGTH_SIZE_T)
            {
                value = (long long)va_arg(parameters, size_t);
            }
            else if (printLength == PRINTF_LENGTH_PTRDIFF_T)
            {
                value = (long long)va_arg(parameters, ptrdiff_t);
            }

            size_t numDigits = 1;
            int    lastDigit = value % 10;

            if (printFlags & PRINTF_FLAG_FORCESIGN && value >= 0)
            {
                written += printfn("+", 1);
                numDigits++;
            }
            else if (printFlags & PRINTF_FLAG_SPACE && value >= 0)
            {
                written += printfn(" ", 1);
                numDigits++;
            }

            if (value < 0)
            {
                written += printfn("-", 1);
                numDigits++;
                value     = -(value / 10);
                lastDigit = -lastDigit;
            }
            else
            {
                value /= 10;
            }

            long long multiple = 1;
            do
            {
                multiple *= 10;
                numDigits++;
            } while (value - (value % multiple) > 0);
            multiple /= 10;

            if (printFlags & PRINTF_FLAG_LEFTPAD ||
                (!(printFlags & PRINTF_FLAG_LJUSTIFY) && width != (size_t)PRINTF_WIDTH_NONE))
            {
                for (size_t i = numDigits; i < width; i++)
                {
                    if (printFlags & PRINTF_FLAG_LEFTPAD)
                    {
                        written += printfn("0", 1);
                    }
                    else
                    {
                        written += printfn(" ", 1);
                    }
                }
            }

            static const char decimalCharacters[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
            while (multiple > 0)
            {
                int digit = value / multiple;
                written += printfn(&decimalCharacters[digit], 1);
                value -= digit * multiple;
                multiple /= 10;
            }

            if (width != 0 || (value == 0 && lastDigit != 0))
            {
                written += printfn(&decimalCharacters[lastDigit], 1);
            }

            if (printFlags & PRINTF_FLAG_LJUSTIFY)
            {
                for (size_t i = numDigits; i < width; i++)
                {
                    written += printfn(" ", 1);
                }
            }
        }
        else if (printFormat == PRINTF_FORMAT_UNSIGNED || printFormat == PRINTF_FORMAT_OCTAL ||
                 printFormat == PRINTF_FORMAT_UHEX || printFormat == PRINTF_FORMAT_UHEXUP ||
                 printFormat == PRINTF_FORMAT_POINTER)
        {
            unsigned long long value = 0;
            if (printFormat == PRINTF_FORMAT_POINTER)
            {
                value = (uintptr_t)va_arg(parameters, void *);
            }
            else if (printLength == PRINTF_LENGTH_CHAR || printLength == PRINTF_LENGTH_SHORT ||
                     printLength == PRINTF_LENGTH_NONE)
            {
                value = (unsigned long long)va_arg(parameters, unsigned int);
            }
            else if (printLength == PRINTF_LENGTH_LONG)
            {
                value = (unsigned long long)va_arg(parameters, unsigned long);
            }
            else if (printLength == PRINTF_LENGTH_LONGLONG)
            {
                value = (unsigned long long)va_arg(parameters, unsigned long long);
            }
            else if (printLength == PRINTF_LENGTH_INTMAX_T)
            {
                value = (unsigned long long)va_arg(parameters, intmax_t);
            }
            else if (printLength == PRINTF_LENGTH_SIZE_T)
            {
                value = (unsigned long long)va_arg(parameters, size_t);
            }
            else if (printLength == PRINTF_LENGTH_PTRDIFF_T)
            {
                value = (unsigned long long)va_arg(parameters, ptrdiff_t);
            }

            int   base = 10;
            char *characterSet =
                (char[]){'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

            size_t numDigits = 1;

            if (printFormat == PRINTF_FORMAT_OCTAL)
            {
                base = 8;
            }
            else if (printFormat == PRINTF_FORMAT_UHEX)
            {
                base = 16;
            }
            else if (printFormat == PRINTF_FORMAT_UHEXUP)
            {
                base         = 16;
                characterSet = (char[]){'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
            }
            else if (printFormat == PRINTF_FORMAT_POINTER)
            {
                base = 16;
                numDigits += 2;
                if (value == 0 && width == (size_t)PRINTF_WIDTH_NONE)
                {
                    if (printFlags & PRINTF_FLAG_LEFTPAD)
                    {
                        for (size_t i = 5; i < width; i++)
                        {
                            written += printfn(" ", 1);
                        }
                    }
                    written += printfn("(nil)", 5);
                    continue;
                }
            }

            int lastDigit = value % base;
            value /= base;

            unsigned long long multiple = 1;
            do
            {
                multiple *= base;
                numDigits++;
            } while (value - (value % multiple) > 0);
            multiple /= base;

            if (width == 0)
            {
                numDigits--;
            }

            if (printFormat == PRINTF_FORMAT_POINTER && printFlags & PRINTF_FLAG_LEFTPAD)
            {
                written += printfn("0x", 2);
            }

            if (printFlags & PRINTF_FLAG_LEFTPAD ||
                (!(printFlags & PRINTF_FLAG_LJUSTIFY) && width != (size_t)PRINTF_WIDTH_NONE))
            {
                for (size_t i = numDigits; i < width; i++)
                {
                    if (printFlags & PRINTF_FLAG_LEFTPAD)
                    {
                        written += printfn("0", 1);
                    }
                    else
                    {
                        written += printfn(" ", 1);
                    }
                }
            }

            if (printFormat == PRINTF_FORMAT_POINTER && !(printFlags & PRINTF_FLAG_LEFTPAD))
            {
                written += printfn("0x", 2);
            }

            if (value != 0)
            {
                while (multiple > 0)
                {
                    int digit = value / multiple;
                    written += printfn(&characterSet[digit], 1);
                    value -= digit * multiple;
                    multiple /= base;
                }
            }

            if (width != 0 || (value == 0 && lastDigit != 0))
            {
                written += printfn(&characterSet[lastDigit], 1);
            }

            if (printFlags & PRINTF_FLAG_LJUSTIFY)
            {
                for (size_t i = numDigits; i < width; i++)
                {
                    written += printfn(" ", 1);
                }
            }
        }
        else if (printFormat == PRINTF_FORMAT_STORE)
        {
            int *outWritten = (int *)va_arg(parameters, void *);
            *outWritten     = written;
        }
        /*
        printFormat == PRINTF_FORMAT_FLOAT || printFormat == PRINTF_FORMAT_FLOATUP ||
                 printFormat == PRINTF_FORMAT_HEXFLOAT || printFormat == PRINTF_FORMAT_HEXFLOATUP ||
                 printFormat == PRINTF_FORMAT_SCI || printFOrmat == PRINTF_FORMAT_SCIUP
        */
        else if (printFormat != PRINTF_FORMAT_ERROR)
        {
            if (printFlags & PRINTF_FLAG_LEFTPAD ||
                (!(printFlags & PRINTF_FLAG_LJUSTIFY) && width != (size_t)PRINTF_WIDTH_NONE))
            {
                for (size_t i = 10; i < width + (size_t)printPrecision; i++)
                {
                    if (printFlags & PRINTF_FLAG_LEFTPAD)
                    {
                        written += printfn("0", 1);
                    }
                    else
                    {
                        written += printfn(" ", 1);
                    }
                }
            }
            written += printfn("NOT.IMPL_", 9);
            written += printfn((char *)&printFormat, 1);
            if (printFlags & PRINTF_FLAG_LJUSTIFY)
            {
                for (size_t i = 10; i < width; i++)
                {
                    written += printfn(" ", 1);
                }
            }
        }
    }
    return written;
}

/**
 * Prints characters using putchar
 *
 * @param[in] data the characters to print
 * @param[in] length the number of characters to print
 *
 * @returns the number of characters printed
 */
static int print(const char *data, size_t length)
{
    const unsigned char *bytes = (const unsigned char *)data;
    for (size_t i = 0; i < length; i++)
        if (putchar(bytes[i]) == EOF) return i;
    return length;
}

/**
 * Prints a formatted string with varargs
 *
 * @param[in] format the format string
 * @param[in] args the format replacements varargs
 *
 * @returns the number of characters written
 */
int vprintf(const char *format, va_list args)
{
    return __print_formatted(print, format, args);
}

/**
 * Prints a formatted string to stdio
 *
 * @param[in] format the format string
 * @param[in] ... the format replacements
 *
 * @returns the number of characters written
 */
int printf(const char *format, ...)
{
    va_list parameters;
    va_start(parameters, format);
    int written = vprintf(format, parameters);
    va_end(parameters);
    return written;
}