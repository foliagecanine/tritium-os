#include <stdio.h>

FILE f;
FILE *fp;

FILE fopen (const char* filename, const char* mode) {
	fp = &f;
	asm volatile("pusha;\
				mov %0,%%ebx;\
				mov %1,%%ecx;\
				mov %2, %%edx;\
				mov $12,%%eax;\
				int $0x80;\
				popa"::"m"(fp),"m"(filename),"m"(mode));
	return f;
}

uint8_t fread (FILE *t, char *buf, uint64_t start, uint64_t len) {
	uint8_t retval;
	uint32_t starth = start>>32;
	uint32_t startl = start&0xFFFFFFFF;
	//uint32_t lenh = len>>32;
	uint32_t lenl = len&0xFFFFFFFF;
	asm volatile("pusha;\
				mov %1,%%ebx;\
				mov %2,%%ecx;\
				mov %3, %%edx;\
				mov %4, %%esi;\
				mov %5, %%edi;\
				mov $13,%%eax;\
				int $0x80;\
				mov %%eax,%0;\
				popa":"=m"(retval):"m"(t),"m"(buf),"m"(starth),"m"(startl),"m"(lenl));
	return retval;
}

uint8_t fwrite (FILE *t, char *buf, uint64_t start, uint64_t len) {
	uint8_t retval;
	uint32_t starth = start>>32;
	uint32_t startl = start&0xFFFFFFFF;
	//uint32_t lenh = len>>32;
	uint32_t lenl = len&0xFFFFFFFF;
	asm volatile("pusha;\
				mov %1,%%ebx;\
				mov %2,%%ecx;\
				mov %3, %%edx;\
				mov %4, %%esi;\
				mov %5, %%edi;\
				mov $14,%%eax;\
				int $0x80;\
				mov %%eax,%0;\
				popa":"=m"(retval):"m"(t),"m"(buf),"m"(starth),"m"(startl),"m"(lenl));
	return retval;
}

FILE fcreate(char *filename) {
	fp = &f;
	asm volatile("pusha;\
				mov %0,%%ebx;\
				mov %1,%%ecx;\
				mov $15,%%eax;\
				int $0x80;\
				popa"::"m"(filename),"m"(fp));
	return f;
}

uint8_t fdelete(char *filename) {
	uint8_t retval;
	asm volatile("pusha;\
				mov %1,%%ebx;\
				mov $16,%%eax;\
				int $0x80;\
				mov %%al,%0;\
				popa":"=m"(retval):"m"(filename));
	return retval;
}

FILE readdir(FILE *d, char* buf, uint32_t n) {
	fp = &f;
	asm volatile("pusha;\
				mov %0,%%ebx;\
				mov %1,%%ecx;\
				mov %2, %%edx;\
				mov %3, %%esi;\
				mov $18,%%eax;\
				int $0x80;\
				popa"::"m"(fp),"m"(d),"m"(buf),"m"(n));
	return f;
}