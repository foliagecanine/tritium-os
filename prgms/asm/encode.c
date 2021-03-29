#include "encode.h"

#define MNEM_MAX 10
#define OPER_MAX 20
#define TEMPSTRING_MAX 200

void estring_db(char *db, bool instr, estring *out) {
	estring_write(out,".db");
	if (instr)
		estring_write(out,"i ");
	else
		estring_write(out," ");
	estring_write(out,db);
	estring_write(out,"\n");
}

void estring_dw(char *dw, bool instr, estring *out) {
	estring_write(out,".dw");
	if (instr)
		estring_write(out,"i ");
	else
		estring_write(out," ");
	estring_write(out,dw);
	estring_write(out,"\n");
}

void estring_dd(char *dd, bool instr, estring *out) {
	estring_write(out,".dd");
	if (instr)
		estring_write(out,"i ");
	else
		estring_write(out," ");
	estring_write(out,dd);
	estring_write(out,"\n");
}

void estring_rel(char *rel, regsz_t size, estring *out) {
	estring_write(out,".r");
	switch(size) {
		case 8:
			estring_write(out,"b ");
			break;
		case 16:
			estring_write(out,"w ");
			break;
		case 32:
			estring_write(out,"d ");
			break;
	}
	estring_write(out,rel);
	estring_write(out,"\n");
}

char temp_tostring[TEMPSTRING_MAX];
unsigned int lineno = 1;

int encode_opcode(optype_t operand, char *op, estring *out) {
	memset(temp_tostring,0,TEMPSTRING_MAX);
	if (operand != NONE) {
		switch(operand) {
			case IMM8:
				estring_db(op, false, out);
				break;
			case IMM16:
				estring_dw(op, false, out);
				break;
			case IMM32:
				estring_dd(op, false, out);
				break;
			case MEM16:
				strncpy(temp_tostring,op+1,TEMPSTRING_MAX);
				if (!strchr(temp_tostring,']')) {
					printf("Error: missing ']'\n");
					return -1;
				}
				*strchr(temp_tostring,']') = 0;
				estring_dw(temp_tostring, false, out);
				break;
			case MEM32:
				strncpy(temp_tostring,op+1,TEMPSTRING_MAX);
				if (!strchr(temp_tostring,']')) {
					printf("Error: missing ']'\n");
					return -1;
				}
				*strchr(temp_tostring,']') = 0;
				estring_dd(temp_tostring, false, out);
				break;
			case REL8:
				estring_rel(op, 8, out);
				break;
			case REL16:
				estring_rel(op, 16, out);
				break;
			case REL32:
				estring_rel(op, 32, out);
				break;
		}
	}
	return 0;
}

// Decode a mnemonic with no operands
int encode_op0(char *mnemonic, estring *out) {
	mnemonic[MNEM_MAX-1] = 0;
	char *p = mnemonic;
	while (*p++)
		*p = (char)tolower(*p);
		
	opcode_t match;
	match.mnemonic = mnemonic;
	match.outop1 = NONE;
	match.outop2 = NONE;
	opcode_t opcode = match_mnem_optypes(match);
	if (!opcode.mnemonic) {
		printf("Unknown instruction: %s\n",mnemonic);
		return -1;
	}
	
	if (opcode.prefix) {
		memset(temp_tostring,0,TEMPSTRING_MAX);
		snprintf(temp_tostring,TEMPSTRING_MAX,"0x%X",opcode.prefix);
		estring_db(temp_tostring, true, out);
	}
	memset(temp_tostring,0,TEMPSTRING_MAX);
	snprintf(temp_tostring,TEMPSTRING_MAX,"0x%X",opcode.opcode);
	estring_db(temp_tostring, opcode.prefix==0, out);
		
	return 0;
}

int encode_op1(char *mnemonic, char *op1, estring *out) {
	mnemonic[MNEM_MAX-1] = 0;
	op1[OPER_MAX-1] = 0;
	char *p = mnemonic;
	while (*p++)
		*p = (char)tolower(*p);
	p = op1;
	while (*p++)
		*p = (char)tolower(*p);
	
	opcode_t match;
	match.mnemonic = mnemonic;
	match.outop1 = get_optype(op1);
	match.outop2 = NONE;
	match.opstring1 = op1;
	opcode_t opcode = match_mnem_1opstr(match);
	if (!opcode.mnemonic) {
		opcode = match_mnem_optypes(match);
		if (!opcode.mnemonic) {
			printf("Unknown instruction: %s %s (%u)\n",mnemonic,op1,match.outop1);
			return -1;
		}
	}
	
	if (opcode.prefix) {
		memset(temp_tostring,0,TEMPSTRING_MAX);
		snprintf(temp_tostring,TEMPSTRING_MAX,"0x%X",opcode.prefix);
		estring_db(temp_tostring, true, out);
	}
	memset(temp_tostring,0,TEMPSTRING_MAX);
	snprintf(temp_tostring,TEMPSTRING_MAX,"0x%X",opcode.opcode);
	estring_db(temp_tostring, opcode.prefix==0, out);
	
	int retval;
	if (retval = encode_opcode(opcode.outop1, op1, out)) {
		printf("Failed to process \"%s %s\"\n",mnemonic,op1);
		return retval;
	}
		
	return 0;
}

int encode_op2(char *mnemonic, char *op1, char *op2, estring *out) {
	mnemonic[MNEM_MAX-1] = 0;
	op1[OPER_MAX-1] = 0;
	op2[OPER_MAX-1] = 0;
	long extract_int;
	char *p = mnemonic;
	while (*p++)
		*p = (char)tolower(*p);
	p = op1;
	while (*p++)
		*p = (char)tolower(*p);
	p = op2;
	while (*p++)
		*p = (char)tolower(*p);
	
	opcode_t match;
	match.mnemonic = mnemonic;
	match.outop1 = get_optype(op1);
	match.outop2 = get_optype(op2);
	match.opstring1 = op1;
	match.opstring2 = op2;
	opcode_t opcode = match_mnem_2opstr(match);
	if (!opcode.mnemonic) {
		opcode = match_mnem_1opstr_2optype(match);
		if (!opcode.mnemonic) {
			opcode = match_mnem_optypes(match);
			if (!opcode.mnemonic) {
				printf("Unknown instruction: %s %s,%s\n",mnemonic,op1,op2);
				return -1;
			}
		}	
	}
	
	if (opcode.prefix) {
		memset(temp_tostring,0,TEMPSTRING_MAX);
		snprintf(temp_tostring,TEMPSTRING_MAX,"0x%X",opcode.prefix);
		estring_db(temp_tostring, true, out);
	}
	memset(temp_tostring,0,TEMPSTRING_MAX);
	snprintf(temp_tostring,TEMPSTRING_MAX,"0x%X",opcode.opcode);
	estring_db(temp_tostring, opcode.prefix==0, out);
	
	int retval;
	if (retval = encode_opcode(opcode.outop1, op1, out)) {
		printf("Failed to process \"%s %s,%s\" at line %u\n",mnemonic,op1,op2,lineno);
		return retval;
	}
	
	if (retval = encode_opcode(opcode.outop2, op2, out)) {
		printf("Failed to process \"%s %s,%s\" at line %u\n",mnemonic,op1,op2,lineno);
		return retval;
	}
		
	return 0;
}

int encode_ops(estring *in, estring *out) {
	char *inptr = estring_getstr(in);
	int retval;
	
	while(true) {
		// Copy any labels
		char *label_end = strchr(inptr,':');
		if (label_end && label_end < strchr(inptr,'\n')) {
			memset(temp_tostring,0,TEMPSTRING_MAX);
			memcpy(temp_tostring,inptr,(size_t)(label_end-inptr));
			estring_write(out,temp_tostring);
			estring_write(out,":\n");
			inptr = label_end + 1;
		}
		
		while (*inptr && (*inptr == ' ' || *inptr == '\t'))
			inptr++;
		
		// Copy any directives. They take the whole line.
		if (*inptr=='.') {
			memset(temp_tostring,0,TEMPSTRING_MAX);
			size_t direct_end = strcspn(inptr,"\n;");
			memcpy(temp_tostring,inptr,direct_end+1);
			estring_write(out,temp_tostring);
			inptr += direct_end+1;
		} else if (*inptr) {
			char mnemonic[MNEM_MAX];
			char op1[OPER_MAX];
			char op2[OPER_MAX];
			memset(mnemonic, 0, MNEM_MAX);
			memset(op1, 0, OPER_MAX);
			memset(op2, 0, OPER_MAX);
			
			size_t nextsym_off = strcspn(inptr," \t\n;");
			memcpy(mnemonic,inptr,nextsym_off);
			inptr += nextsym_off;
			
			while (*inptr && (*inptr == ' ' || *inptr == '\t'))
				inptr++;
				
			if (*inptr == '\n' || *inptr == ';' || !*inptr) {
				// No operands
				// If just a label, don't encode
				if (mnemonic[0]) {
					if (retval = encode_op0(mnemonic, out))
						return retval;
				}
				
				if (*inptr == ';') {
					while (*inptr && *inptr != '\n')
						inptr++;
				}
				if (*inptr == '\n')
					inptr++;
			} else {
				// At least one operand
				nextsym_off = strcspn(inptr," \t\n;,");
				memcpy(op1,inptr,nextsym_off);
				inptr += nextsym_off;
				
				while (*inptr && (*inptr == ' ' || *inptr == '\t'))
					inptr++;
					
				if (*inptr == '\n' || *inptr == ';' || !*inptr) {
					// 1 operand
					if (retval = encode_op1(mnemonic, op1, out))
						return retval;
					
					if (*inptr == ';') {
						while (*inptr && *inptr != '\n')
							inptr++;
						inptr++;
					}
					if (*inptr == '\n')
						inptr++;
				} else {
					// Two operands
					inptr++;
					nextsym_off = strcspn(inptr," \t\n;");
					memcpy(op2,inptr,nextsym_off);
					inptr += nextsym_off;
					
					while (*inptr && *inptr != '\n')
						inptr++;
						
					if (retval = encode_op2(mnemonic, op1, op2, out))
						return retval;
					
					inptr++;
				}
			}
		}
		
		// End of input
		if (!(*inptr))
			return 0;
		
		lineno++;
	}
}