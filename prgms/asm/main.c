#include <stdio.h>
#include <stdlib.h>
#include "encode.h"
#include "macros.h"
#include "elf.h"

char *helpinfo = "usage: asm [options...] [-o outfile] filename\n\
\n\
Options:\n\
	--help, -h, -?	print this help info\n\
	--version, -v	print version information\n\
	-p				process mnemonics only\n";

char *version = "0.1";

enum error {
	ERROR_SUCCESS,
	ERROR_NOINPUT,
	ERROR_UNRECOGNISED,
	ERROR_MISSINGPARAM,
	ERROR_MISSINGFILE,
	ERROR_OUTOFMEM,
	ERROR_PROCESSING,
};

#define TOTAL_ARGS 7
enum option {
	OPTION_HELP_1,
	OPTION_HELP_2,
	OPTION_HELP_3,
	OPTION_VERSION_1,
	OPTION_VERSION_2,
	OPTION_OUTPUTFILE,
	OPTION_PROCESSOPS,
};

char *options[TOTAL_ARGS] = {
	"--help",
	"-h",
	"-?",
	"--version",
	"-v",
	"-o",
	"-p",
};

int get_option_index(char *arg) {
	for (int i = 0; i < TOTAL_ARGS; i++)
			if (!strcmp(arg,options[i]))
				return i;
	return -1;
}

char def_out_buffer[4096];
bool option_only_process_ops = false;

char *default_output(char *inputname) {
	char *lastdot = strrchr(inputname, '.');
	char *outputname = def_out_buffer;
	if (lastdot == NULL) {
		strcpy(outputname, inputname);
		strcpy(outputname+strlen(inputname),".o");
	} else {
		strcpy(outputname, inputname);
		if (option_only_process_ops)
			strcpy(strrchr(outputname, '.')+1, "ops");
		else
			strcpy(strrchr(outputname, '.')+1, "o");
	}
	return outputname;
}

char *outputfile = NULL;
char *inputfile = NULL;
char *inputdata;

int main(int argc, char **argv) {
	if (argc==1) {
		printf("%s: no input\n",argv[0]);
		return ERROR_NOINPUT;
	}
	for (int i = 1; i < argc; i++) {
		int opt_index = get_option_index(argv[i]);
		if (opt_index == -1 && (argv[i][0] == '-' || i != argc - 1)) {
			printf("Error: unrecognised option: %s\n", argv[i]);
			return ERROR_UNRECOGNISED;
		}
		switch((enum option)opt_index) {
			case OPTION_HELP_1:
			case OPTION_HELP_2:
			case OPTION_HELP_3:
				printf(helpinfo);
				return ERROR_SUCCESS;
			case OPTION_VERSION_1:
			case OPTION_VERSION_2:
				printf("asm.prg version %s\n",version);
				return ERROR_SUCCESS;
			case OPTION_OUTPUTFILE:
				if (++i == argc || argv[i][0] == '-') {
					printf("Error: -o missing parameter\n");
					return ERROR_MISSINGPARAM;
				}
				outputfile = argv[i];
				break;
			case OPTION_PROCESSOPS:
				option_only_process_ops = true;
				break;
			default:
				if (i == argc - 1) {
					inputfile = argv[i];
					break;
				}
				printf("Error parsing argument %s\n",argv[i]);
				return ERROR_UNRECOGNISED;
		}
	}
	
	if (inputfile == NULL) {
		printf("%s: no input\n",argv[0]);
		return ERROR_NOINPUT;
	}
	
	if (outputfile == NULL)
		outputfile = default_output(inputfile);
	
	FILE *infile = openfile(inputfile,"r");
	if (!infile->valid) {
		printf("Error: cannot open input file.\n");
		return ERROR_MISSINGFILE;
	}
	
	printf("Input file: %s\n",inputfile);
	printf("Output file: %s\n",outputfile);
	
	inputdata = malloc(infile->size+1);
	
	if (!inputdata) {
		printf("Error: cannot allocate memory.\n");
		return ERROR_OUTOFMEM;
	}
	
	inputdata[infile->size] = 0;
	
	if (readfile(infile, inputdata, 0, infile->size)) {
		printf("Error: failed to read input file.\n");
		return ERROR_MISSINGFILE;
	}
	
	int retval;
	estring *in_data = estring_create();
	
	if (!in_data) {
		printf("Error: could not create estring\n");
		return ERROR_OUTOFMEM;
	}
	
	if (!estring_write(in_data,inputdata)) {
		printf("Error: could not write estring\n");
		return ERROR_OUTOFMEM;
	}
	estring_write(in_data,"\n"); // Append newline to ensure it processes correctly.
	
	free(inputdata);
	
	estring *encoded_ops = estring_create();
	if (!encoded_ops) {
		printf("Error: could not create estring\n");
		return ERROR_OUTOFMEM;
	}
	
	if (retval = encode_ops(in_data,encoded_ops))
		return retval;
	
	estring_delete(in_data);
	
	if (option_only_process_ops) {
		FILE *outfile = openfile(outputfile,"w");
		if (!outfile->valid) {
			printf("Error: cannot open output file.\n");
			return ERROR_MISSINGFILE;
		}
		
		if (writefile(outfile, estring_getstr(encoded_ops), 0, estring_len(encoded_ops))) {
			printf("Error: failed to write output file.\n");
			return ERROR_MISSINGFILE;
		}
		
		printf("Done.\n");
		return ERROR_SUCCESS;
	}
	
	size_t len;
	char *encoded = encode_macros(encoded_ops,&len);
	
	if (!encoded) {
		printf("Error when processing macros.\n");
		return ERROR_PROCESSING;
	}
	
	estring_delete(encoded_ops);
	
	void *elf = encode_elf(encoded, len, &len);
	
	FILE *outfile = openfile(outputfile,"w");
	if (!outfile->valid) {
		printf("Error: cannot open output file.\n");
		return ERROR_MISSINGFILE;
	}
	
	if (writefile(outfile, elf, 0, len)) {
		printf("Error: failed to write output file.\n");
		return ERROR_MISSINGFILE;
	}
	
	printf("Done.\n");
	return ERROR_SUCCESS;
}
