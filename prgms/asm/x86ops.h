#ifndef _X86OPS_H
#define _X86OPS_H

#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include "asmtypes.h"

#define BIT(x) (1 << x)

typedef enum { NONE = 0, SREG, REGRM32, REGRM16, REGRM8, IMM32, IMM16, IMM8, MEM32, MEM16, REL32, REL16, REL8, LABEL } optype_t;

// REGRM32: eax, ecx, [eax], ...								mov eax,ebx
// REGRM16: ax, cx, [ax], ...									mov ax,bx
// REGRM8 : al, ah, [ah], ...									mov al,bl
// SREG   : cs, ds												mov eax,cs
// IMM8   : Values between 0x00 and 0xFF						mov al,0x12
// IMM16  : Values between 0x0000 and 0xFFFF					mov ax,0x1234
// IMM32  : Values between 0x00000000 and 0xFFFFFFFF			mov eax,0x12345678
// MEM16  : 16-bit memory addresses								mov eax,[0x1234]
// MEM32  : 32-bit memory addresses								mov eax,[0x12345678]
// REL32  : 32-bit relative memory address						jmp 0x12345678
// REL16  : 16-bit relative memory address						jmp 0x1234
// REL8   : 8-bit relative memory address						jmp 0x12
// LABEL  : Label-encoded address								jmp label

typedef struct {
	char *mnemonic;
	char *opstring1;
	char *opstring2;
	uint8_t opcode;
	uint8_t prefix;
	uint8_t modreg;
	optype_t outop1;
	optype_t outop2;
	uint16_t optype1;
	uint16_t optype2;
} opcode_t;

typedef struct {
	uint8_t mod:2;
	uint8_t rm:3;
	uint8_t reg:3;
} modrm_t;

/**
 * Match mnemonic with one operand string.
 * 
 * @param match Opcode structure with mnemonic and operand information.
 * @returns On success returns the matched opcode structure. On failure returns an opcode structure with mnemonic set to NULL.
 */
opcode_t match_mnem_1opstr(opcode_t match);

/**
 * Match mnemonic with two operand strings.
 * 
 * @param match Opcode structure with mnemonic and operand information.
 * @returns On success returns the matched opcode structure. On failure returns an opcode structure with mnemonic set to NULL.
 */
opcode_t match_mnem_2opstr(opcode_t match);

/**
 * Match mnemonic with operand types.
 * 
 * @param match Opcode structure with mnemonic and operand type information.
 * @returns On success returns the matched opcode structure. On failure returns an opcode structure with mnemonic set to NULL.
 */
opcode_t match_mnem_1opstr_2optype(opcode_t match);

/**
 * Match mnemonic with operand types.
 * 
 * @param match Opcode structure with mnemonic and operand type information.
 * @returns On success returns the matched opcode structure. On failure returns an opcode structure with mnemonic set to NULL.
 */
opcode_t match_mnem_optypes(opcode_t match);

/**
 * Get the operand type from the operand string.
 * 
 * @param op Pointer to the operand string.
 * @returns On success returns the operand type. On failure returns NONE.
 */
optype_t get_optype(char *op);

#endif