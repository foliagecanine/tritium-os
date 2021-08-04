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

int __printf_template(bool (*printfn)(const char *, size_t), const char* restrict format, va_list parameters) {
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
			if (!printfn(format, amount))
				return -1;
			format += amount;
			written += amount;
			continue;
		}

		const char* format_begun_at = format++;
		long long vararg = 0;
		bool format_specified = false;

		if (*format == 'l') {
			format++;
			if (*format == 'l') {
				format++;
				vararg = (long long)va_arg(parameters, long long);
			} else {
				vararg = (long long)(long)va_arg(parameters, long);
			}
			format_specified = true;
		}

		if (*format == 'c') {
			format++;
			char c = (char) va_arg(parameters, int /* char promotes to int */);
			if (!maxrem) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!printfn(&c, sizeof(c)))
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
			if (!printfn(str, len))
				return -1;
			written += len;
		} else if (*format == '#') { // For backwards compatibility only. Will be removed later. Use a length attribute (if applicable) and 'x' (or 'X') as proper format specifier
			format++;
			unsigned long long num = (unsigned long long)va_arg(parameters, unsigned long long);
			if (!num) {
				if (!printfn("0",1))
					return -1;
				written++;
			} else {
				unsigned long long tnum = num; //Count number of digits to make a new array
				int count = 0;
				while (tnum>0) {
					tnum = tnum / 16;
					++count;
				}
				int size = count;

				char outlist[size];
				char numlist[] = {"0123456789ABCDEF"};
				unsigned long long dig;
				tnum = num;
				count = 0;
				while(tnum>0) {
					dig = tnum % 16;
					outlist[size-count-1] = numlist[dig];
					tnum = tnum / 16;
					++count;
				}
				if (!printfn(outlist, size))
					return -1;
				written+=size;
			}
		} else if (*format == 'p') { // p format is same as x format except it takes a vararg of type void *, it redefines 0 as (nil), and it prepends "0x" to non-null pointers
			format++;
			#if __SIZEOF_POINTER__ == 8
				unsigned long long num = (unsigned long long)va_arg(parameters, void *);
			#else
				unsigned long long num = (unsigned long long)(unsigned long)va_arg(parameters, void *);
			#endif
			if (!num) {
				if (!printfn("(nil)",5))
					return -1;
				written++;
			} else {
				//Count number of digits to make a new array
				unsigned long long tnum = num;
				int count = 0;
				while (tnum>0) {
					tnum = tnum / 16;
					++count;
				}
				int size = count+2;

				char outlist[size];
				char numlist[] = {"0123456789ABCDEF"};
				unsigned long long dig;
				tnum = num;
				count = 0;
				while(tnum>0) {
					dig = tnum % 16;
					outlist[size-count-1] = numlist[dig];
					tnum = tnum / 16;
					++count;
				}
				
				outlist[0]='0';
				outlist[1]='x';
				
				if (!printfn(outlist, size))
					return -1;
				written+=size;
			}
		} else if (*format == 'x' || *format == 'X') {
			if (!format_specified)
				vararg = (long long)(int)va_arg(parameters, int);
			
			char numlist[16];
			if (*format == 'X')
				memcpy(numlist,"0123456789ABCDEF",16);
			else
				memcpy(numlist,"0123456789abcdef",16);
			format++;
			if (!vararg) {
				if (!printfn("0",1))
					return -1;
				written++;
			} else {
				//Count number of digits to make a new array
				unsigned long long tnum = (unsigned long long)vararg; 
				int count = 0;
				while (tnum>0) {
					tnum = tnum / 16;
					++count;
				}
				int size = count;

				char outlist[size];
				
				unsigned long long dig;
				tnum = vararg;
				count = 0;
				while(tnum>0) {
					dig = tnum % 16;
					outlist[size-count-1] = numlist[dig];
					tnum = tnum / 16;
					++count;
				}
				if (!printfn(outlist, size))
					return -1;
				written+=size;
			}
		} else if (*format == 'u') {
			if (!format_specified)
				vararg = (long long)(int)va_arg(parameters, int);
			
			format++;
			if (!vararg) {
				if (!printfn("0",1))
					return -1;
				written++;
			} else {
				// Count number of digits to create a new array
				unsigned long long tnum = (unsigned long long)vararg; // Cast to unsigned since format is u
				unsigned long long dig;
				int count = 0;
				while (tnum>0) {
					tnum /= 10;
					++count;
				}
				int size = count;

				char outlist[size];
				char numlist[] = {"0123456789"};
				tnum = vararg;
				count = 0;
				while(tnum>0) {
					dig = tnum % 10;
					outlist[size-count-1] = numlist[dig];
					tnum /= 10;
					++count;
				}
				
				if (!printfn(outlist, size))
					return -1;
				written+=size;
			}
		} else if (*format == 'd') {
			if (!format_specified)
				vararg = (long long)(int)va_arg(parameters, int);
			
			format++;
			if (!vararg) {
				if (!printfn("0",1))
					return -1;
				written++;
			} else {
				// Convert negative number to positive number
				if (vararg < 0) {
					printfn("-",1);
					vararg = ~vararg;
					vararg++;
				}
				
				// Count number of digits to create a new array
				unsigned long long tnum = (unsigned long long)vararg; // Cast to unsigned since format is u
				unsigned long long dig;
				int count = 0;
				while (tnum>0) {
					tnum /= 10;
					++count;
				}
				int size = count;

				char outlist[size];
				char numlist[] = {"0123456789"};
				tnum = vararg;
				count = 0;
				while(tnum>0) {
					dig = tnum % 10;
					outlist[size-count-1] = numlist[dig];
					tnum /= 10;
					++count;
				}
				
				if (!printfn(outlist, size))
					return -1;
				written+=size;
			}
		} else if (*format == '$') { // This implemation exists solely for backwards compatibility. Existing uses should be updated.
			format++;
			long long num = (long long)(long)va_arg(parameters, long); // For backwards compatibility for now. Should be reset to int.
			if (!num) {
				if (!printfn("0",1))
					return -1;
				written++;
			} else {
				long long tnum = num; //Count number of digits to make a new array
				int count = 0;
				while (tnum>0) {
					tnum /= 10;
					++count;
				}
				int size = count;
				
				if (num<0)
					size++; // Add space for negative in char array

				char outlist[size];
				char numlist[] = {"0123456789"};
				long long dig;
				tnum = num;
				count = 0;
				while(tnum>0) {
					dig = tnum % 10;
					outlist[size-count-1] = numlist[dig];
					tnum /= 10;
					++count;
				}
				
				if (num<0)
					outlist[0] = '-'; // Char array fills backwards, so just put negative at first char
				
				if (!printfn(outlist, size))
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
			if (!printfn(format, len))
				return -1;
			written += len;
			format += len;
		}
	}

	return written;
}

int printf(const char *format, ...) {
	va_list parameters;
	va_start(parameters, format);
	int written = __printf_template(print, format, parameters);
	va_end(parameters);
	return written;
}