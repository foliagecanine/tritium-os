#include "x86ops.h"

char *regs32[] = {"eax","ecx","edx","ebx","esp","ebp","esi","edi"};
char *regs16[] = {"ax","cx","dx","bx","sp","bp","si","di"};
char *regs8[] = {"al","cl","dl","bl","ah","ch","dh","bh"};

const opcode_t opcodes[] = {
	//Mnemonic	opstr1	os2  opcode prefix modreg	outop1	outop2	optype1...		optype2...
	{"add", 	"al",  	NULL, 0x04, 0,	0xff,		NONE,	IMM8, 	BIT(REGRM8),	BIT(IMM8)},
	{"add", 	"ax",	NULL, 0x05, 0x66,	0xff,	NONE, 	IMM16,	BIT(REGRM16),	BIT(IMM8) | BIT(IMM16)},
	{"add", 	"eax",	NULL, 0x05, 0,	0xff,		NONE, 	IMM32,	BIT(REGRM32),	BIT(IMM8) | BIT(IMM16) | BIT(IMM32)},
	
	{"push", 	"es",	NULL, 0x06, 0,	0xff,		NONE, 	NONE, 	BIT(SREG), 	BIT(NONE)},
	{"pop", 	"es",	NULL, 0x07, 0,	0xff,		NONE, 	NONE, 	BIT(SREG), 	BIT(NONE)},
	
	{"or", 		"al",	NULL, 0x0C, 0,	0xff,		NONE, 	IMM8, 	BIT(REGRM8), 	BIT(IMM8)},
	{"or", 		"ax",	NULL, 0x0D, 0x66,	0xff,	NONE, 	IMM16, 	BIT(REGRM16), 	BIT(IMM8) | BIT(IMM16)},
	{"or", 		"eax",	NULL, 0x0D, 0,	0xff,		NONE, 	IMM32, 	BIT(REGRM32), 	BIT(IMM8) | BIT(IMM16) | BIT(IMM32)},
	
	{"push", 	"cs",	NULL, 0x0E, 0,	0xff,		NONE, 	NONE, 	BIT(SREG), 	BIT(NONE)},
	
	{"adc", 	"al",	NULL, 0x14, 0,	0xff,		NONE, 	IMM8, 	BIT(REGRM8), 	BIT(IMM8)},
	{"adc", 	"ax",	NULL, 0x15, 0x66,	0xff,	NONE, 	IMM16, 	BIT(REGRM16), 	BIT(IMM8) | BIT(IMM16)},
	{"adc", 	"eax",	NULL, 0x15, 0,	0xff,		NONE, 	IMM32, 	BIT(REGRM32), 	BIT(IMM8) | BIT(IMM16) | BIT(IMM32)},
	
	{"push", 	"ss",	NULL, 0x16, 0,	0xff,		NONE, 	NONE, 	BIT(SREG), 	BIT(NONE)},
	{"pop", 	"ss",	NULL, 0x17, 0,	0xff,		NONE, 	NONE, 	BIT(SREG), 	BIT(NONE)},
	
	{"sbb", 	"al",	NULL, 0x1C, 0,	0xff,		NONE, 	IMM8, 	BIT(REGRM8), 	BIT(IMM8)},
	{"sbb", 	"ax",	NULL, 0x1D, 0x66,	0xff,	NONE, 	IMM16, 	BIT(REGRM16), 	BIT(IMM8) | BIT(IMM16)},
	{"sbb", 	"eax",	NULL, 0x1D, 0,	0xff,		NONE, 	IMM32, 	BIT(REGRM32), 	BIT(IMM8) | BIT(IMM16) | BIT(IMM32)},
	
	{"push", 	"ds",	NULL, 0x1E, 0,	0xff,		NONE, 	NONE, 	BIT(SREG), 	BIT(NONE)},
	{"pop", 	"ds",	NULL, 0x1F, 0,	0xff,		NONE, 	NONE, 	BIT(SREG), 	BIT(NONE)},
	
	{"and", 	"al",	NULL, 0x24, 0,	0xff,		NONE, 	IMM8, 	BIT(REGRM8), 	BIT(IMM8)},
	{"and", 	"ax",	NULL, 0x25, 0x66,	0xff,	NONE, 	IMM32, 	BIT(REGRM16), 	BIT(IMM8) | BIT(IMM16)},
	{"and", 	"eax",	NULL, 0x25, 0,	0xff,		NONE, 	IMM32, 	BIT(REGRM32), 	BIT(IMM8) | BIT(IMM16) | BIT(IMM32)},
	
	{"daa",		NULL,	NULL, 0x27, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),	BIT(NONE)},
	
	{"sub", 	"al",	NULL, 0x2C, 0,	0xff,		NONE, 	IMM8, 	BIT(REGRM8), 	BIT(IMM8)},
	{"sub", 	"ax",	NULL, 0x2D, 0x66,	0xff,	NONE, 	IMM16, 	BIT(REGRM16), 	BIT(IMM8) | BIT(IMM16)},
	{"sub", 	"eax",	NULL, 0x2D, 0,	0xff,		NONE, 	IMM32, 	BIT(REGRM32), 	BIT(IMM8) | BIT(IMM16) | BIT(IMM32)},
	
	{"das",		NULL,	NULL, 0x2F, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),	BIT(NONE)},
	
	{"xor", 	"al",	NULL, 0x34, 0,	0xff,		NONE, 	IMM8, 	BIT(REGRM8), 	BIT(IMM8)},
	{"xor", 	"ax",	NULL, 0x35, 0x66,	0xff,	NONE, 	IMM16, 	BIT(REGRM16), 	BIT(IMM8) | BIT(IMM16)},
	{"xor", 	"eax",	NULL, 0x35, 0,	0xff,		NONE, 	IMM32, 	BIT(REGRM32), 	BIT(IMM8) | BIT(IMM16) | BIT(IMM32)},
	
	{"aaa",		NULL,	NULL, 0x37, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	
	{"cmp", 	"al",	NULL, 0x3C, 0,	0xff,		NONE, 	IMM8, 	BIT(REGRM8), 	BIT(IMM8)},
	{"cmp", 	"ax",	NULL, 0x3D, 0x66,	0xff,	NONE, 	IMM16, 	BIT(REGRM16), 	BIT(IMM8) | BIT(IMM16)},
	{"cmp", 	"eax",	NULL, 0x3D, 0,	0xff,		NONE, 	IMM32, 	BIT(REGRM32), 	BIT(IMM8) | BIT(IMM16) | BIT(IMM32)},
	
	{"aas",		NULL,	NULL, 0x3F, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	{"pusha",	NULL,	NULL, 0x60, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	{"pushad",	NULL,	NULL, 0x60, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	{"popa",	NULL,	NULL, 0x61, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	{"popad",	NULL,	NULL, 0x61, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	
	{"jo",		NULL,	NULL, 0x70, 0,	0xff,		REL8,	NONE,	BIT(IMM8) | BIT(IMM16) | BIT(IMM32) | BIT(LABEL), BIT(NONE)},
	{"jno",		NULL,	NULL, 0x71, 0,	0xff,		REL8,	NONE,	BIT(IMM8) | BIT(IMM16) | BIT(IMM32) | BIT(LABEL), BIT(NONE)},
	{"jb",		NULL,	NULL, 0x72, 0,	0xff,		REL8,	NONE,	BIT(IMM8) | BIT(IMM16) | BIT(IMM32) | BIT(LABEL), BIT(NONE)},
	{"jnae",	NULL,	NULL, 0x72, 0,	0xff,		REL8,	NONE,	BIT(IMM8) | BIT(IMM16) | BIT(IMM32) | BIT(LABEL), BIT(NONE)},
	{"jc",		NULL,	NULL, 0x72, 0,	0xff,		REL8,	NONE,	BIT(IMM8) | BIT(IMM16) | BIT(IMM32) | BIT(LABEL), BIT(NONE)},
	{"jnb",		NULL,	NULL, 0x73, 0,	0xff,		REL8,	NONE,	BIT(IMM8) | BIT(IMM16) | BIT(IMM32) | BIT(LABEL), BIT(NONE)},
	{"jnc",		NULL,	NULL, 0x73, 0,	0xff,		REL8,	NONE,	BIT(IMM8) | BIT(IMM16) | BIT(IMM32) | BIT(LABEL), BIT(NONE)},
	{"jae",		NULL,	NULL, 0x73, 0,	0xff,		REL8,	NONE,	BIT(IMM8) | BIT(IMM16) | BIT(IMM32) | BIT(LABEL), BIT(NONE)},
	{"jz",		NULL,	NULL, 0x74, 0,	0xff,		REL8,	NONE,	BIT(IMM8) | BIT(IMM16) | BIT(IMM32) | BIT(LABEL), BIT(NONE)},
	{"je",		NULL,	NULL, 0x74, 0,	0xff,		REL8,	NONE,	BIT(IMM8) | BIT(IMM16) | BIT(IMM32) | BIT(LABEL), BIT(NONE)},
	{"jnz",		NULL,	NULL, 0x75, 0,	0xff,		REL8,	NONE,	BIT(IMM8) | BIT(IMM16) | BIT(IMM32) | BIT(LABEL), BIT(NONE)},
	{"jne",		NULL,	NULL, 0x75, 0,	0xff,		REL8,	NONE,	BIT(IMM8) | BIT(IMM16) | BIT(IMM32) | BIT(LABEL), BIT(NONE)},
	{"jbe",		NULL,	NULL, 0x76, 0,	0xff,		REL8,	NONE,	BIT(IMM8) | BIT(IMM16) | BIT(IMM32) | BIT(LABEL), BIT(NONE)},
	{"jna",		NULL,	NULL, 0x76, 0,	0xff,		REL8,	NONE,	BIT(IMM8) | BIT(IMM16) | BIT(IMM32) | BIT(LABEL), BIT(NONE)},
	{"jnbe",	NULL,	NULL, 0x77, 0,	0xff,		REL8,	NONE,	BIT(IMM8) | BIT(IMM16) | BIT(IMM32) | BIT(LABEL), BIT(NONE)},
	{"ja",		NULL,	NULL, 0x77, 0,	0xff,		REL8,	NONE,	BIT(IMM8) | BIT(IMM16) | BIT(IMM32) | BIT(LABEL), BIT(NONE)},
	{"js",		NULL,	NULL, 0x78, 0,	0xff,		REL8,	NONE,	BIT(IMM8) | BIT(IMM16) | BIT(IMM32) | BIT(LABEL), BIT(NONE)},
	{"jns",		NULL,	NULL, 0x79, 0,	0xff,		REL8,	NONE,	BIT(IMM8) | BIT(IMM16) | BIT(IMM32) | BIT(LABEL), BIT(NONE)},
	{"jp",		NULL,	NULL, 0x7A, 0,	0xff,		REL8,	NONE,	BIT(IMM8) | BIT(IMM16) | BIT(IMM32) | BIT(LABEL), BIT(NONE)},
	{"jpe",		NULL,	NULL, 0x7A, 0,	0xff,		REL8,	NONE,	BIT(IMM8) | BIT(IMM16) | BIT(IMM32) | BIT(LABEL), BIT(NONE)},
	{"jnp",		NULL,	NULL, 0x7B, 0,	0xff,		REL8,	NONE,	BIT(IMM8) | BIT(IMM16) | BIT(IMM32) | BIT(LABEL), BIT(NONE)},
	{"jpo",		NULL,	NULL, 0x7B, 0,	0xff,		REL8,	NONE,	BIT(IMM8) | BIT(IMM16) | BIT(IMM32) | BIT(LABEL), BIT(NONE)},
	{"jl",		NULL,	NULL, 0x7C, 0,	0xff,		REL8,	NONE,	BIT(IMM8) | BIT(IMM16) | BIT(IMM32) | BIT(LABEL), BIT(NONE)},
	{"jnge",	NULL,	NULL, 0x7C, 0,	0xff,		REL8,	NONE,	BIT(IMM8) | BIT(IMM16) | BIT(IMM32) | BIT(LABEL), BIT(NONE)},
	{"jnl",		NULL,	NULL, 0x7D, 0,	0xff,		REL8,	NONE,	BIT(IMM8) | BIT(IMM16) | BIT(IMM32) | BIT(LABEL), BIT(NONE)},
	{"jge",		NULL,	NULL, 0x7D, 0,	0xff,		REL8,	NONE,	BIT(IMM8) | BIT(IMM16) | BIT(IMM32) | BIT(LABEL), BIT(NONE)},
	{"jle",		NULL,	NULL, 0x7E, 0,	0xff,		REL8,	NONE,	BIT(IMM8) | BIT(IMM16) | BIT(IMM32) | BIT(LABEL), BIT(NONE)},
	{"jng",		NULL,	NULL, 0x7E, 0,	0xff,		REL8,	NONE,	BIT(IMM8) | BIT(IMM16) | BIT(IMM32) | BIT(LABEL), BIT(NONE)},
	{"jnle",	NULL,	NULL, 0x7F, 0,	0xff,		REL8,	NONE,	BIT(IMM8) | BIT(IMM16) | BIT(IMM32) | BIT(LABEL), BIT(NONE)},
	{"jg",		NULL,	NULL, 0x7F, 0,	0xff,		REL8,	NONE,	BIT(IMM8) | BIT(IMM16) | BIT(IMM32) | BIT(LABEL), BIT(NONE)},
	
	{"add",		NULL,	NULL, 0x80, 0,	0,			REGRM8,	IMM8,	BIT(REGRM8),	BIT(IMM8)},	
	{"or",		NULL,	NULL, 0x80, 0,	1,			REGRM8,	IMM8,	BIT(REGRM8),	BIT(IMM8)},	
	{"adc",		NULL,	NULL, 0x80, 0,	2,			REGRM8,	IMM8,	BIT(REGRM8),	BIT(IMM8)},	
	{"sbb",		NULL,	NULL, 0x80, 0,	3,			REGRM8,	IMM8,	BIT(REGRM8),	BIT(IMM8)},	
	{"and",		NULL,	NULL, 0x80, 0,	4,			REGRM8,	IMM8,	BIT(REGRM8),	BIT(IMM8)},	
	{"sub",		NULL,	NULL, 0x80, 0,	5,			REGRM8,	IMM8,	BIT(REGRM8),	BIT(IMM8)},	
	{"xor",		NULL,	NULL, 0x80, 0,	6,			REGRM8,	IMM8,	BIT(REGRM8),	BIT(IMM8)},	
	{"cmp",		NULL,	NULL, 0x80, 0,	7,			REGRM8,	IMM8,	BIT(REGRM8),	BIT(IMM8)},	
	
	{"add",		NULL,	NULL, 0x81, 0x66,	0,		REGRM16,IMM16,	BIT(REGRM16),	BIT(IMM8) | BIT(IMM16)},	
	{"or",		NULL,	NULL, 0x81, 0x66,	1,		REGRM16,IMM16,	BIT(REGRM16),	BIT(IMM8) | BIT(IMM16)},	
	{"adc",		NULL,	NULL, 0x81, 0x66,	2,		REGRM16,IMM16,	BIT(REGRM16),	BIT(IMM8) | BIT(IMM16)},	
	{"sbb",		NULL,	NULL, 0x81, 0x66,	3,		REGRM16,IMM16,	BIT(REGRM16),	BIT(IMM8) | BIT(IMM16)},	
	{"and",		NULL,	NULL, 0x81, 0x66,	4,		REGRM16,IMM16,	BIT(REGRM16),	BIT(IMM8) | BIT(IMM16)},	
	{"sub",		NULL,	NULL, 0x81, 0x66,	5,		REGRM16,IMM16,	BIT(REGRM16),	BIT(IMM8) | BIT(IMM16)},	
	{"xor",		NULL,	NULL, 0x81, 0x66,	6,		REGRM16,IMM16,	BIT(REGRM16),	BIT(IMM8) | BIT(IMM16)},	
	{"cmp",		NULL,	NULL, 0x81, 0x66,	7,		REGRM16,IMM16,	BIT(REGRM16),	BIT(IMM8) | BIT(IMM16)},	
	
	{"add",		NULL,	NULL, 0x81, 0,	0,			REGRM32,IMM32,	BIT(REGRM32),	BIT(IMM8) | BIT(IMM16) | BIT(IMM32)},	
	{"or",		NULL,	NULL, 0x81, 0,	1,			REGRM32,IMM32,	BIT(REGRM32),	BIT(IMM8) | BIT(IMM16) | BIT(IMM32)},	
	{"adc",		NULL,	NULL, 0x81, 0,	2,			REGRM32,IMM32,	BIT(REGRM32),	BIT(IMM8) | BIT(IMM16) | BIT(IMM32)},	
	{"sbb",		NULL,	NULL, 0x81, 0,	3,			REGRM32,IMM32,	BIT(REGRM32),	BIT(IMM8) | BIT(IMM16) | BIT(IMM32)},	
	{"and",		NULL,	NULL, 0x81, 0,	4,			REGRM32,IMM32,	BIT(REGRM32),	BIT(IMM8) | BIT(IMM16) | BIT(IMM32)},	
	{"sub",		NULL,	NULL, 0x81, 0,	5,			REGRM32,IMM32,	BIT(REGRM32),	BIT(IMM8) | BIT(IMM16) | BIT(IMM32)},	
	{"xor",		NULL,	NULL, 0x81, 0,	6,			REGRM32,IMM32,	BIT(REGRM32),	BIT(IMM8) | BIT(IMM16) | BIT(IMM32)},	
	{"cmp",		NULL,	NULL, 0x81, 0,	7,			REGRM32,IMM32,	BIT(REGRM32),	BIT(IMM8) | BIT(IMM16) | BIT(IMM32)},	
	
	{"nop",		NULL,	NULL, 0x90, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	{"nopw",	NULL,	NULL, 0x90, 0x66,0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	
	{"xchg",	"ax",	"ax", 0x90, 0x66,	0xff,	NONE, 	NONE, 	BIT(REGRM16),	BIT(REGRM16)},
	{"xchg",	"cx",	"ax", 0x91, 0x66,	0xff,	NONE, 	NONE, 	BIT(REGRM16),	BIT(REGRM16)},
	{"xchg",	"dx",	"ax", 0x92, 0x66,	0xff,	NONE, 	NONE, 	BIT(REGRM16),	BIT(REGRM16)},
	{"xchg",	"bx",	"ax", 0x93, 0x66,	0xff,	NONE, 	NONE, 	BIT(REGRM16),	BIT(REGRM16)},
	{"xchg",	"sp",	"ax", 0x94, 0x66,	0xff,	NONE, 	NONE, 	BIT(REGRM16),	BIT(REGRM16)},
	{"xchg",	"bp",	"ax", 0x95, 0x66,	0xff,	NONE, 	NONE, 	BIT(REGRM16),	BIT(REGRM16)},
	{"xchg",	"si",	"ax", 0x96, 0x66,	0xff,	NONE, 	NONE, 	BIT(REGRM16),	BIT(REGRM16)},
	{"xchg",	"di",	"ax", 0x97, 0x66,	0xff,	NONE, 	NONE, 	BIT(REGRM16),	BIT(REGRM16)},
	
	{"xchg",	"eax",	"eax", 0x90, 0,	0xff,		NONE, 	NONE, 	BIT(REGRM32),	BIT(REGRM32)},
	{"xchg",	"ecx",	"eax", 0x91, 0,	0xff,		NONE, 	NONE, 	BIT(REGRM32),	BIT(REGRM32)},
	{"xchg",	"edx",	"eax", 0x92, 0,	0xff,		NONE, 	NONE, 	BIT(REGRM32),	BIT(REGRM32)},
	{"xchg",	"ebx",	"eax", 0x93, 0,	0xff,		NONE, 	NONE, 	BIT(REGRM32),	BIT(REGRM32)},
	{"xchg",	"esp",	"eax", 0x94, 0,	0xff,		NONE, 	NONE, 	BIT(REGRM32),	BIT(REGRM32)},
	{"xchg",	"ebp",	"eax", 0x95, 0,	0xff,		NONE, 	NONE, 	BIT(REGRM32),	BIT(REGRM32)},
	{"xchg",	"esi",	"eax", 0x96, 0,	0xff,		NONE, 	NONE, 	BIT(REGRM32),	BIT(REGRM32)},
	{"xchg",	"edi",	"eax", 0x97, 0,	0xff,		NONE, 	NONE, 	BIT(REGRM32),	BIT(REGRM32)},
	
	{"cbw",		NULL,	NULL, 0x98, 0x66,	0xff,	NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	{"cbwe",	NULL,	NULL, 0x98, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	{"cwd",		NULL,	NULL, 0x99, 0x66,	0xff,	NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	{"cdq",		NULL,	NULL, 0x99, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	{"pushf",	NULL,	NULL, 0x9C, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	{"pushfd",	NULL,	NULL, 0x9C, 0,	0xff,		NONE, 	NONE,	BIT(NONE),		BIT(NONE)},
	{"popf",	NULL,	NULL, 0x9D, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	{"popfd",	NULL,	NULL, 0x9D, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	{"sahf",	NULL,	NULL, 0x9E, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	{"lahf",	NULL,	NULL, 0x9F, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	
	{"mov", 	"al",	NULL, 0xA0, 0,	0xff,		NONE, 	MEM32, 	BIT(REGRM8), 	BIT(MEM16) | BIT(MEM32) | BIT(LABEL)},
	{"mov", 	"ax",	NULL, 0xA1, 0x66,	0xff,	NONE, 	MEM32, 	BIT(REGRM16), 	BIT(MEM16) | BIT(MEM32) | BIT(LABEL)},
	{"mov", 	"eax",	NULL, 0xA1, 0,	0xff,		NONE, 	MEM32, 	BIT(REGRM32), 	BIT(MEM16) | BIT(MEM32) | BIT(LABEL)},
	
	{"mov", 	"al",	NULL, 0xB0, 0,	0xff,		NONE, 	IMM8, 	BIT(REGRM32), 	BIT(IMM8)},
	{"mov", 	"cl",	NULL, 0xB1, 0,	0xff,		NONE, 	IMM8, 	BIT(REGRM32), 	BIT(IMM8)},
	{"mov", 	"dl",	NULL, 0xB2, 0,	0xff,		NONE, 	IMM8, 	BIT(REGRM32), 	BIT(IMM8)},
	{"mov", 	"bl",	NULL, 0xB3, 0,	0xff,		NONE, 	IMM8, 	BIT(REGRM32), 	BIT(IMM8)},
	{"mov", 	"ah",	NULL, 0xB4, 0,	0xff,		NONE, 	IMM8, 	BIT(REGRM32), 	BIT(IMM8)},
	{"mov", 	"ch",	NULL, 0xB5, 0,	0xff,		NONE, 	IMM8, 	BIT(REGRM32), 	BIT(IMM8)},
	{"mov", 	"dh",	NULL, 0xB6, 0,	0xff,		NONE, 	IMM8, 	BIT(REGRM32), 	BIT(IMM8)},
	{"mov", 	"bh",	NULL, 0xB7, 0,	0xff,		NONE, 	IMM8, 	BIT(REGRM32), 	BIT(IMM8)},
	
	{"mov", 	"ax",	NULL, 0xB8, 0x66,	0xff,	NONE, 	IMM16, 	BIT(REGRM32), 	BIT(IMM8) | BIT(IMM16)},
	{"mov", 	"cx",	NULL, 0xB9, 0x66,	0xff,	NONE, 	IMM16, 	BIT(REGRM32), 	BIT(IMM8) | BIT(IMM16)},
	{"mov", 	"dx",	NULL, 0xBA, 0x66,	0xff,	NONE, 	IMM16, 	BIT(REGRM32), 	BIT(IMM8) | BIT(IMM16)},
	{"mov", 	"bx",	NULL, 0xBB, 0x66,	0xff,	NONE, 	IMM16, 	BIT(REGRM32), 	BIT(IMM8) | BIT(IMM16)},
	{"mov", 	"sp",	NULL, 0xBC, 0x66,	0xff,	NONE, 	IMM16, 	BIT(REGRM32), 	BIT(IMM8) | BIT(IMM16)},
	{"mov", 	"bp",	NULL, 0xBD, 0x66,	0xff,	NONE, 	IMM16, 	BIT(REGRM32), 	BIT(IMM8) | BIT(IMM16)},
	{"mov", 	"si",	NULL, 0xBE, 0x66,	0xff,	NONE, 	IMM16, 	BIT(REGRM32), 	BIT(IMM8) | BIT(IMM16)},
	{"mov", 	"di",	NULL, 0xBF, 0x66,	0xff,	NONE, 	IMM16, 	BIT(REGRM32), 	BIT(IMM8) | BIT(IMM16)},
	
	{"mov", 	"eax",	NULL, 0xB8, 0,	0xff,		NONE, 	IMM32, 	BIT(REGRM32), 	BIT(IMM8) | BIT(IMM16) | BIT(IMM32)},
	{"mov", 	"ecx",	NULL, 0xB9, 0,	0xff,		NONE, 	IMM32, 	BIT(REGRM32), 	BIT(IMM8) | BIT(IMM16) | BIT(IMM32)},
	{"mov", 	"edx",	NULL, 0xBA, 0,	0xff,		NONE, 	IMM32, 	BIT(REGRM32), 	BIT(IMM8) | BIT(IMM16) | BIT(IMM32)},
	{"mov", 	"ebx",	NULL, 0xBB, 0,	0xff,		NONE, 	IMM32, 	BIT(REGRM32), 	BIT(IMM8) | BIT(IMM16) | BIT(IMM32)},
	{"mov", 	"esp",	NULL, 0xBC, 0,	0xff,		NONE, 	IMM32, 	BIT(REGRM32), 	BIT(IMM8) | BIT(IMM16) | BIT(IMM32)},
	{"mov", 	"ebp",	NULL, 0xBD, 0,	0xff,		NONE, 	IMM32, 	BIT(REGRM32), 	BIT(IMM8) | BIT(IMM16) | BIT(IMM32)},
	{"mov", 	"esi",	NULL, 0xBE, 0,	0xff,		NONE, 	IMM32, 	BIT(REGRM32), 	BIT(IMM8) | BIT(IMM16) | BIT(IMM32)},
	{"mov", 	"edi",	NULL, 0xBF, 0,	0xff,		NONE, 	IMM32, 	BIT(REGRM32), 	BIT(IMM8) | BIT(IMM16) | BIT(IMM32)},
	
	{"ret",		NULL,	NULL, 0xC3, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	{"retd",	NULL,	NULL, 0xC3, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	{"iret",	NULL,	NULL, 0xCF, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	{"iretd",	NULL,	NULL, 0xCF, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	{"leave",	NULL,	NULL, 0xC9, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	
	{"int3",	NULL,	NULL, 0xCC, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	{"int", 	"3", 	NULL, 0xCC, 0,	0xff,		NONE, 	NONE, 	BIT(IMM8),		BIT(NONE)},
	{"int", 	NULL, 	NULL, 0xCD, 0,	0xff,		IMM8, 	NONE, 	BIT(IMM8),		BIT(NONE)},
	
	{"into",	NULL,	NULL, 0xCE, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	{"aam",		NULL,	NULL, 0x0A, 0xD4, 0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)}, //? Is this really a prefixed opcode
	{"aad",		NULL,	NULL, 0x0A, 0xD5, 0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)}, //? Is this really a prefixed opcode
	{"salc",	NULL,	NULL, 0xD6, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	{"setalc",	NULL,	NULL, 0xD6, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	
	{"jmp",		NULL,	NULL, 0xE9, 0,	0xff,		REL32,	NONE,	BIT(IMM32) | BIT(LABEL), BIT(NONE)},
	{"jmp",		NULL,	NULL, 0xE9, 0x66,	0xff,	REL16,	NONE,	BIT(IMM16),		BIT(NONE)},
	{"jmp",		NULL,	NULL, 0xEB, 0,	0xff,		REL8,	NONE,	BIT(IMM8),		BIT(NONE)},
	
	{"in", 		"al", 	"dx", 0xEC, 0,	0xff,		NONE, 	NONE, 	BIT(REGRM8), 	BIT(REGRM16)},
	{"in", 		"ax", 	"dx", 0xED, 0x66,	0xff,	NONE, 	NONE, 	BIT(REGRM16), 	BIT(REGRM16)},
	{"in", 		"eax", 	"dx", 0xED, 0,	0xff,		NONE, 	NONE, 	BIT(REGRM32), 	BIT(REGRM16)},
	{"out", 	"dx", 	"al", 0xEE, 0,	0xff,		NONE, 	NONE, 	BIT(REGRM16), 	BIT(REGRM8)},
	{"out", 	"dx", 	"ax", 0xEF, 0x66,	0xff,	NONE,	NONE, 	BIT(REGRM16), 	BIT(REGRM16)},
	{"out", 	"dx", 	"eax", 0xEF, 0,	0xff,		NONE,	NONE, 	BIT(REGRM16), 	BIT(REGRM32)},
	
	{"repnz",	NULL,	NULL, 0xF2, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	{"repne",	NULL,	NULL, 0xF2, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	{"rep",		NULL,	NULL, 0xF3, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	{"repz",	NULL,	NULL, 0xF3, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	{"repe",	NULL,	NULL, 0xF3, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	{"hlt",		NULL,	NULL, 0xF4, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	{"cmc",		NULL,	NULL, 0xF5, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	{"clc",		NULL,	NULL, 0xF8, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	{"stc",		NULL,	NULL, 0xF9, 0,	0xff,		NONE,	NONE, 	BIT(NONE),		BIT(NONE)},
	{"cli",		NULL,	NULL, 0xFA, 0,	0xff,		NONE,	NONE, 	BIT(NONE),		BIT(NONE)},
	{"sti",		NULL,	NULL, 0xFB, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	{"cld",		NULL,	NULL, 0xFC, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	{"std",		NULL,	NULL, 0xFD, 0,	0xff,		NONE, 	NONE, 	BIT(NONE),		BIT(NONE)},
	{NULL, 		NULL,	NULL, 0,	0,	0xff,	 	NONE, 	NONE,	BIT(NONE), 		BIT(NONE)},
};

opcode_t match_mnem_1opstr(opcode_t match) {
	int i = 0;
	while (opcodes[i].mnemonic) {
		if (opcodes[i].opstring1) {
			if (!memcmp(match.mnemonic, opcodes[i].mnemonic, strlen(opcodes[i].mnemonic))) {
				if (!memcmp(match.opstring1, opcodes[i].opstring1, strlen(opcodes[i].opstring1))) {
					return opcodes[i];
				}
			}
		}
		i++;
	}
	return opcodes[i];
}

opcode_t match_mnem_2opstr(opcode_t match) {
	int i = 0;
	while (opcodes[i].mnemonic) {
		if (opcodes[i].opstring1) {
			if (opcodes[i].opstring2) {
				if (!memcmp(match.mnemonic, opcodes[i].mnemonic, strlen(opcodes[i].mnemonic))) {
					if (!memcmp(match.opstring1, opcodes[i].opstring1, strlen(opcodes[i].opstring1))) {
						if (!memcmp(match.opstring2, opcodes[i].opstring2, strlen(opcodes[i].opstring2))) {
							return opcodes[i];
						}
					}
				}
			}
		}
		i++;
	}
	return opcodes[i];
}

opcode_t match_mnem_1opstr_2optype(opcode_t match) {
	int i = 0;
	while (opcodes[i].mnemonic) {
		if (opcodes[i].opstring1) {
			if (!memcmp(match.mnemonic, opcodes[i].mnemonic, strlen(opcodes[i].mnemonic))) {
				if (!memcmp(match.opstring1, opcodes[i].opstring1, strlen(opcodes[i].opstring1))) {
					if (BIT(match.outop2) & opcodes[i].optype2)
						return opcodes[i];
				}
			}
		}
		i++;
	}
	return opcodes[i];
}

opcode_t match_mnem_optypes(opcode_t match) {
	int i = 0;
	while (opcodes[i].mnemonic) {
		if (!memcmp(match.mnemonic, opcodes[i].mnemonic, strlen(opcodes[i].mnemonic))) {
			if (BIT(match.outop1) & opcodes[i].optype1 && BIT(match.outop2) & opcodes[i].optype2) {
				if (!opcodes[i].opstring1)
					return opcodes[i];
			}
		}
		i++;
	}
	return opcodes[i];
}

regsz_t get_regsz(const char *s) {
	for (int i = 0; i < 8; i++) {
		if (!strncmp(s,regs32[i],3))
			return 32;
		if (!strncmp(s,regs16[i],2))
			return 16;
		if (!strncmp(s,regs8[i],2))
			return 8;
	}
	return 0;
}

int numscale(char *n) {
	unsigned long immval;
	
	if (n[0]=='0') {
		if (n[1]=='x') {
			immval = strtoul(n+2,NULL,16);
		} else if (n[1]=='b') {
			immval = strtoul(n+2,NULL,2);
		} else {
			// Assume octal. If the value was zero,
			// it will be treated as octal, but that's
			// OK since it will still be zero.
			immval = strtoul(n+1,NULL,8);
		}
	} else {
		if (isdigit(n[0])) {
			immval = strtoul(n,NULL,10);
		} else {
			return 0;
		}
	}
	
	if (immval < 0x100)
		return 8;
	if (immval < 0x10000)
		return 16;
	return 32;
}

optype_t get_optype(char *op) {
	// Immediate value
	int scale = numscale(op);
	regsz_t regsz;
	switch(scale) {
		case 8:
			return IMM8;
		case 16:
			return IMM16;
		case 32:
			return IMM32;
		default:
			break;
	}
		
	// Memory-based (ModRM)
	if (*op == '[') {
		op++;
		scale = numscale(op);
		
		char *endbracket = strchr(op,']');
		if (!endbracket) {
			printf("Error: no ending bracket: %s\n",op);
			return NONE;
		}
		
		switch(scale) {
			case 8:
			case 16:
				return MEM16;
			case 32:
				return MEM32;
			default:
				break;
		}
		
		regsz = get_regsz(op);
		switch(regsz) {
			case 8:
				return REGRM8;
			case 16:
				return REGRM16;
			case 32:
				return REGRM32;
			default:
				break;
		}
		
		// Assume label
		return LABEL;
	}
	
	regsz = get_regsz(op);
	switch(regsz) {
		case 8:
			return REGRM8;
		case 16:
			return REGRM16;
		case 32:
			return REGRM32;
		default:
			break;
	}
		
	// Assume label
	return LABEL;
}

int encode_modrm(char *op1, char *op2, optype_t type1, optype_t type2) {
	modrm_t modrm;
	char **reglist;
	char *plusminus;
	if (type1 == REGRM8)
		reglist = regs8;
	else if (type1 == REGRM16)
		reglist = regs16;
	else if (type1 == REGRM32)
		reglist = regs32;
	else {
		printf("Error encoding ModRM\n");
		return 0;
	}
	
	int i;
	for (i = 0; i < 8; i++) {
		if (!strncmp(op1,reglist[i],strlen(reglist[i])))
			break;
	}
	modrm.reg = i;
	
	if (type1 == REGRM8)
		reglist = regs8;
	else if (type1 == REGRM16)
		reglist = regs16;
	else if (type1 == REGRM32)
		reglist = regs32;
	else {
		printf("Error encoding ModRM\n");
		return 0;
	}
	
	for (i = 0; i < 8; i++) {
		if (!strncmp(op1,reglist[i],strlen(reglist[i]))) {
			modrm.mod = 3;
			modrm.rm = i;
			return (BYTE)((modrm.mod << 6) | (modrm.reg << 3) | modrm.rm);
		}
	}
	
	printf("How to handle %s?\n",op2);
	exit(99);//for(;;);
}