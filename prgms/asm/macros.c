#include "macros.h"

#define LOAD_ADDRESS 0x8040000

unsigned long multibase(char *n, char **endptr) {
	unsigned long immval;
	
	if (n[0]=='0') {
		if (n[1]=='x') {
			immval = strtoul(n+2,endptr,16);
		} else if (n[1]=='b') {
			immval = strtoul(n+2,endptr,2);
		} else if (isdigit(n[1])) {
			immval = strtoul(n+1,endptr,8);
		} else {
			// Handle zero
			*endptr = n + 1;
			return 0;
		}
	} else {
		if (isdigit(n[0])) {
			immval = strtoul(n,endptr,10);
		} else {
			*endptr = n;
			return 0;
		}
	}
	
	return immval;
}

char *encoded_macros;
size_t len_encoded;

int write_byte(char b) {
	if (encoded_macros == NULL) {
		encoded_macros = malloc(1);
		if (encoded_macros == NULL)
			return -1;
	} else {
		encoded_macros = realloc(encoded_macros,len_encoded+1);
	}
	*(encoded_macros + len_encoded) = b;
	len_encoded++;
	return 0;
}

uint32_t get_labeladdr(char *start, char *ptr, bool *error) {
	char *endptr = ptr;
	while (*endptr && *endptr != ' ' && *endptr != '\t' && *endptr != '\n')
		endptr++;
	size_t stringlen = endptr-ptr;
	
	//printf("Label size: %u\n",stringlen);
	
	char *searchstr = malloc(stringlen+2);
	if (!searchstr) {
		printf("Error: out of memory.\n");
		*error = true;
		return 0;
	}
	memcpy(searchstr,ptr,stringlen);
	searchstr[stringlen] = ':';
	searchstr[stringlen+1] = 0;
	
	//printf("Searching for label \"%s\"\n",searchstr,ptr);
	
	char *searchptr = start;
	
	uint32_t labelptr = LOAD_ADDRESS;
	while(true) {
		while (*searchptr && (*searchptr == ' ' || *searchptr == '\t'))
			putchar(*searchptr++);
			
		if (!*searchptr) {
			*error = true;
			printf("Error finding label %s\n",searchstr);
			return 0;
		}
			
		if (!strncmp(searchptr,searchstr,stringlen+1))
			break;
		if (searchptr[0]=='.' && searchptr[1]) {
			switch(searchptr[2]) {
				case 'b':
					labelptr++;
					break;
				case 'w':
					labelptr += 2;
					break;
				case 'd':
					labelptr += 4;
					break;
				default:
					printf("Error finding label %s\n",searchstr);
					free(searchstr);
					*error = true;
					return 0;
			}
		}
		
		searchptr += strcspn(searchptr,"\n");
		if (*searchptr)
			searchptr++;
	}
	
	free(searchstr);
	*error = false;
	//printf("Label address: %X\n",labelptr);
	return labelptr;
}

void *encode_macros(estring *in, size_t *len_out) {
	encoded_macros = NULL;
	len_encoded = 0;
	uint32_t currbyte = LOAD_ADDRESS;
	uint32_t instrbyte = currbyte;
	char *inptr = estring_getstr(in);
	char *endptr;
	char shortstr[5];
	unsigned long val;
	
	while(true) {
		while (*inptr && (*inptr == ' ' || *inptr == '\t'))
			inptr++;
			
		if (!strncmp(inptr,".db",3)) {
			inptr += 3;
			if (*inptr == 'i') {
				instrbyte = currbyte;
				inptr++;
			}
			while (true) {
				while (*inptr && (*inptr == ' ' || *inptr == '\t'))
					inptr++;
				if (!*inptr || *inptr == '\n')
					break;
				val = multibase(inptr,&endptr);
				if (inptr == endptr) {
					bool error;
					uint32_t labeladdr = get_labeladdr(estring_getstr(in),inptr,&error);
					if (error) {
						printf("Error: failed to encode macros.\n");
						return NULL;
					}
					val = labeladdr;
				}
				val &= 0xff;
				if (write_byte((char)(val & 0xff)))
					return NULL;
				currbyte++;
				inptr = endptr;
			}
			inptr++;
		} else if (!strncmp(inptr,".dw",3)) {
			inptr += 3;
			if (*inptr == 'i') {
				instrbyte = currbyte;
				inptr++;
			}
			while (true) {
				while (*inptr && (*inptr == ' ' || *inptr == '\t'))
					inptr++;
				if (*inptr == '\n')
					break;
				val = multibase(inptr,&endptr);
				if (inptr == endptr) {
					bool error;
					uint32_t labeladdr = get_labeladdr(estring_getstr(in),inptr,&error);
					if (error) {
						printf("Error: failed to encode macros.\n");
						return NULL;
					}
					val = labeladdr;
				}
				val &= 0xffff;
				if (write_byte((char)(val & 0xff)))
					return NULL;
				if (write_byte((char)((val >> 8) & 0xff)))
					return NULL;
				currbyte += 2;
				inptr = endptr;
			}
			inptr++;
		} else if (!strncmp(inptr,".dd",3)) {
			inptr += 3;
			if (*inptr == 'i') {
				instrbyte = currbyte;
				inptr++;
			}
			while (true) {
				while (*inptr && (*inptr == ' ' || *inptr == '\t'))
					inptr++;
				if (*inptr == '\n')
					break;
				val = multibase(inptr,&endptr);
				if (inptr == endptr) {
					bool error;
					uint32_t labeladdr = get_labeladdr(estring_getstr(in),inptr,&error);
					if (error) {
						printf("Error: failed to encode macros.\n");
						return NULL;
					}
					val = labeladdr;
					while (*inptr && *inptr != ' ' && *inptr != '\t' && *inptr != '\n')
						inptr++;
					endptr = inptr;
				}
				if (write_byte((char)(val & 0xff)))
					return NULL;
				if (write_byte((char)((val >> 8) & 0xff)))
					return NULL;
				if (write_byte((char)((val >> 16) & 0xff)))
					return NULL;
				if (write_byte((char)((val >> 24) & 0xff)))
					return NULL;
				currbyte += 4;
				inptr = endptr;
			}
			inptr++;
		} else if (!strncmp(inptr,".rb",3)) {
			inptr += 3;
			while (*inptr && (*inptr == ' ' || *inptr == '\t'))
					inptr++;
			if (*inptr == '\n') {
				printf("Error: illegal newline found at relative address.\n\tRelative addressing is reserved for internal use only.\n");
				return NULL;
			}
			
			//printf("Current address: %X\n",instrbyte);
			
			bool error;
			uint32_t labeladdr = get_labeladdr(estring_getstr(in),inptr,&error);
			if (error)
				return NULL;
			
			int64_t offset = labeladdr;
			offset -= instrbyte + 2;
			
			if (offset > INT8_MAX || offset < INT8_MIN) {
				printf("Error: label offset outside of range -128 to 127: %lld\n",offset);
				return NULL;
			}
			
			int8_t adjusted_offset = offset;
			
			if (write_byte((char)(adjusted_offset & 0xff)))
				return NULL;
				
			inptr += strcspn(inptr,"\n");
			if (*inptr)
				inptr++;
			
			currbyte++;
		} else if (!strncmp(inptr,".rw",3)) {
			inptr += 3;
			while (*inptr && (*inptr == ' ' || *inptr == '\t'))
					inptr++;
			if (*inptr == '\n') {
				printf("Error: illegal newline found at relative address.\n\tRelative addressing is reserved for internal use only.\n");
				return NULL;
			}
			
			//printf("Current address: %X\n",instrbyte);
			
			bool error;
			uint32_t labeladdr = get_labeladdr(estring_getstr(in),inptr,&error);
			if (error)
				return NULL;
			
			int64_t offset = labeladdr;
			offset -= instrbyte + 3;
			
			if (offset > INT16_MAX || offset < INT16_MIN) {
				printf("Error: label offset outside of range -32768 to 32767: %lld\n",offset);
				return NULL;
			}
			
			int16_t adjusted_offset = offset;
			
			if (write_byte((char)(adjusted_offset & 0xff)))
				return NULL;
			if (write_byte((char)((adjusted_offset >> 8) & 0xff)))
				return NULL;
			
			inptr += strcspn(inptr,"\n");
			if (*inptr)
				inptr++;
				
			currbyte += 2;
		} else if (!strncmp(inptr,".rd",3)) {
			inptr += 3;
			while (*inptr && (*inptr == ' ' || *inptr == '\t'))
					inptr++;
			if (*inptr == '\n') {
				printf("Error: illegal newline found at relative address.\n\tRelative addressing is reserved for internal use only.\n");
				return NULL;
			}
			
			//printf("Current address: %X\n",instrbyte);
			
			bool error;
			uint32_t labeladdr = get_labeladdr(estring_getstr(in),inptr,&error);
			if (error)
				return NULL;
			
			int64_t offset = labeladdr;
			offset -= instrbyte + 5;
			
			if (offset > INT32_MAX || offset < INT32_MIN) {
				printf("Error: label offset outside of range -2147483648 to 2147483647: %lld\n",offset);
				return NULL;
			}
			
			int32_t adjusted_offset = offset;
			
			if (write_byte((char)(adjusted_offset & 0xff)))
				return NULL;
			if (write_byte((char)((adjusted_offset >> 8) & 0xff)))
				return NULL;
			if (write_byte((char)((adjusted_offset >> 16) & 0xff)))
				return NULL;
			if (write_byte((char)((adjusted_offset >> 24) & 0xff)))
				return NULL;
				
			inptr += strcspn(inptr,"\n");
			if (*inptr)
				inptr++;
				
			currbyte += 4;
		} else {
			while (*inptr && *inptr != '\n')
				inptr++;
			inptr++;
		}
		
		// End of input
		if (!(*inptr)) {
			*len_out = len_encoded;
			return encoded_macros;
		}
	}
}

int encode_labels(estring *in, estring *out) {
	char *inptr = estring_getstr(in);
}