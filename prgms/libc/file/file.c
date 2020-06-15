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

void fread (FILE *t, char *buf, uint64_t start, uint64_t len) {
	uint32_t starth = start>>32;
	uint32_t startl = start&0xFFFFFFFF;
	//uint32_t lenh = len>>32;
	uint32_t lenl = len&0xFFFFFFFF;
	asm volatile("pusha;\
				mov %0,%%ebx;\
				mov %1,%%ecx;\
				mov %2, %%edx;\
				mov %3, %%esi;\
				mov %4, %%edi;\
				mov $13,%%eax;\
				int $0x80;\
				popa"::"m"(t),"m"(buf),"m"(starth),"m"(startl),"m"(lenl));
}