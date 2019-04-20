#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static bool print(const char* data, size_t length) {
	const unsigned char* bytes = (const unsigned char*) data;
	for (size_t i = 0; i < length; i++)
		if (putchar(bytes[i]) == EOF)
			return false;
	return true;
}

int printf(const char* restrict format, ...) {
	va_list parameters;
	va_start(parameters, format);

	int written = 0;

	while (*format != '\0') {
		size_t maxrem = INT_MAX - written;

		if (format[0] != '%' || format[1] == '%') {
			if (format[0] == '%')
				format++;
			size_t amount = 1;
			while (format[amount] && format[amount] != '%')
				amount++;
			if (maxrem < amount) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(format, amount))
				return -1;
			format += amount;
			written += amount;
			continue;
		}

		const char* format_begun_at = format++;

		if (*format == 'c') {
			format++;
			char c = (char) va_arg(parameters, int /* char promotes to int */);
			if (!maxrem) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(&c, sizeof(c)))
				return -1;
			written++;
		} else if (*format == 's') {
			format++;
			const char* str = va_arg(parameters, const char*);
			size_t len = strlen(str);
			if (maxrem < len) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(str, len))
				return -1;
			written += len;
		} else if (*format == '#') {
			format++;
			const long num = (long)va_arg(parameters, int);
			if (num==0) {
				if (!print("0",1))
					return -1;
				written++;
			} else {
				long tnum = num; //Count number of digits to make a new array
				long dig;
				int count = 0;
				while (tnum>0) {
					tnum = tnum / 16;
					++count;
				}
				int size = count;

				char outlist[count];
				char numlist[] = {"0123456789ABCDEF"};
				char convchar;
				tnum = num;
				count = 0;
				while(tnum>0) {
					dig = tnum % 16;
					convchar = numlist[dig];
					outlist[size-count-1] = convchar;
					tnum = tnum / 16;
					++count;
				}
				if (!print(outlist, size))
					return -1;
				written+=size;
			}
		} else if (*format == 'd') {
			format++;
			const long num = (long)va_arg(parameters, int);
			if (num==0) {
				if (!print("0",1))
					return -1;
				written++;
			} else {
				long tnum = num; //Count number of digits to make a new array
				long dig;
				int count = 0;
				while (tnum>0) {
					tnum = tnum / 10;
					++count;
				}
				int size = count;

				char outlist[count];
				char numlist[] = {"0123456789"};
				char convchar;
				tnum = num;
				count = 0;
				while(tnum>0) {
					dig = tnum % 10;
					convchar = numlist[dig];
					outlist[size-count-1] = convchar;
					tnum = tnum / 10;
					++count;
				}
				if (!print(outlist, size))
					return -1;
				written+=size;
			}
		} else {
			format = format_begun_at;
			size_t len = strlen(format);
			if (maxrem < len) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(format, len))
				return -1;
			written += len;
			format += len;
		}
	}

	va_end(parameters);
	return written;
}
