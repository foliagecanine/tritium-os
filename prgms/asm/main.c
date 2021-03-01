#include <stdio.h>
#include <stdlib.h>

char *helpinfo = "usage: asm [options...] [-o outfile] filename\n\
\n\
Options:\n\
	--help, -h, -?	print this help info\n\
	--version, -v	print version information\n";

char *version = "0.1";

enum error {
	ERROR_SUCCESS,
	ERROR_NOINPUT,
	ERROR_UNRECOGNISED,
	ERROR_MISSINGPARAM,
	ERROR_MISSINGFILE,
	ERROR_OUTOFMEM,
};

#define TOTAL_ARGS 6
enum option {
	OPTION_HELP_1,
	OPTION_HELP_2,
	OPTION_HELP_3,
	OPTION_VERSION_1,
	OPTION_VERSION_2,
	OPTION_OUTPUTFILE,
};

char *options[TOTAL_ARGS] = {
	"--help",
	"-h",
	"-?",
	"--version",
	"-v",
	"-o",
};

int get_option_index(char *arg) {
	for (int i = 0; i < TOTAL_ARGS; i++)
			if (!strcmp(arg,options[i]))
				return i;
	return -1;
}

char def_out_buffer[4096];

char *default_output(char *inputname) {
	char *lastdot = strrchr(inputname, '.');
	char *outputname = def_out_buffer;
	if (lastdot == NULL) {
		strcpy(outputname, inputname);
		strcpy(outputname+strlen(inputname),".o");
	} else {
		strcpy(outputname, inputname);
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
	
	FILE *infile = fopen(inputfile,"r");
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
	
	if (fread(infile, inputdata, 0, infile->size)) {
		printf("Error: failed to read input file.\n");
		return ERROR_MISSINGFILE;
	}
	
	printf("%s",inputdata);
}
