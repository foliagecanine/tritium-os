#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unixfile.h>

#define MAX_FILE_SIZE 512
#define MAX_FILENAME_LENGTH 4096

void process_args(int argc, char **argv, char *fname, bool *ignore);
void read_file(char *fname, char *buf);
void print_99bottles(void);

void main(uint32_t argc, char **argv)
{
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));

    char buf[MAX_FILE_SIZE + 1]     = {0};
    char fname[MAX_FILENAME_LENGTH] = {0};
    bool ignore                     = false;

    process_args(argc, argv, fname, &ignore);

    read_file(fname, buf);

    for (uint16_t i = 0; i < MAX_FILE_SIZE; i++)
    {
        if (buf[i] == 'H')
            printf("Hello World\n");
        else if (buf[i] == 'Q')
            printf("%s", buf);
        else if (buf[i] == '9')
        {
            print_99bottles();
        }
        else if (buf[i] != '+' && buf[i] != ' ' && buf[i] != '\n')
        {
            if (!buf[i]) exit(0);
            if (!ignore)
            {
                printf("Syntax error in %u (ERR 127)\n", i);
                exit(127);
            }
        }
    }
    exit(0);
}

void process_args(int argc, char **argv, char *fname, bool *ignore)
{
    for (uint32_t i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-?") || !strcmp(argv[i], "--help"))
        {
            printf("HQ9COMP: An HQ9+ compiler/interpreter\n"
                   "Version 1.0\n\n"
                   "Usage: HQ9COMP [OPTION] SOURCEFILE\n\n"
                   "Options:\n"
                   "-?   --help : Display help\n"
                   "-i --ignore : Ignore syntax errors\n");
            exit(0);
        }
        else if (!strcmp(argv[i], "-i") || !strcmp(argv[i], "--ignore"))
        {
            *ignore = true;
        }
        else
        {
            strcpy(fname, argv[i]);
        }
    }
}

void read_file(char *fname, char *buf)
{
    if (strlen(fname) == 0)
    {
        printf("File argument required! Use --help for help.\n");
        exit(0);
    }

    FILE *f = fopen(fname, "r");
    if (!f)
    {
        printf("Could not open file (ERR 1).\n");
        exit(1);
    }

    if (fread(buf, sizeof(char), MAX_FILE_SIZE, f) == 0)
    {
        printf("Could not read file or file empty (ERR 2).\n");
        exit(2);
    }
}

void print_99bottles()
{
    for (int j = 99; j > 1; j--)
    {
        printf("%u bottles of beer on the wall, %u bottles of beer.\n", j, j);
        printf("Take one down, pass it around. %u bottles of beer on the wall.\n\n", j - 1);
    }
    printf("1 bottle of beer on the wall, 1 bottle of beer.\n");
    printf("Take one down, pass it around. No bottles of beer on the wall.\n");
}