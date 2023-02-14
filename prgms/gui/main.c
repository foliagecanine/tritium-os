#include <gui.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tty.h>

uint8_t min      = 0;
uint8_t selected = 0;
uint8_t count    = 0;
FILE *  currdir;
char    name[16][31];
char    buf[31];
char    cd[4096];
char    program[4096];

uint8_t disks[8];
uint8_t numdisks;
uint8_t diskselected = 0;

bool diskselector()
{
    for (uint8_t i = 0; i < numdisks; i++)
    {
        if (i == diskselected)
        {
            terminal_setcolor(0x9F);
        }
        else
        {
            terminal_setcolor(0xF0);
        }
        terminal_goto(36, 9 + (i));
        if (disks[i] != 0xFF)
            printf(" %c:/ ", 65 + disks[i]);
        else
        {
            printf("     ");
        }
    }
    terminal_setcolor(0xF0);
    terminal_goto(24, 20);
    while (1)
    {
        uint8_t g = getkey();
        if (g == 0x50)
        {
            if (diskselected < numdisks - 1) diskselected++;
            return false;
        }
        if (g == 0xC8)
        {
            if (diskselected > 0) diskselected--;
            return false;
        }
        if (g == 0x1C)
        {
            getchar();
            cd[0] = 65 + disks[diskselected];
            cd[1] = ':';
            cd[2] = '/';
            cd[3] = 0;
            return true;
        }
        if (g == 0x01)
        {
            return true;
        }
    }
}

bool resetselection = true;
char testdisk[]     = "#:/";

bool programselector()
{
    for (uint8_t i = min; i < min + 12; i++)
    {
        if (i == selected)
        {
            terminal_setcolor(0x9F);
        }
        else
        {
            terminal_setcolor(0xF0);
        }
        terminal_goto(24, 8 + (i - min));
        if (i < count)
        {
            printf(" %s ", name[i]);
        }
        else
        {
            printf("                                ");
        }
    }
    terminal_setcolor(0xF0);
    terminal_goto(24, 20);
    if (count - min > 12)
        printf("               \031\031\031                ");
    else
    {
        printf("        End of directory       ");
    }
    while (1)
    {
        uint8_t g = getkey();
        if (g == 0x50)
        {
            if (selected < count - 1) selected++;
            if (selected >= min + 12) min++;
            return false;
        }
        if (g == 0x48)
        {
            if (selected > 0) selected--;
            if (selected < min) min--;
            return false;
        }
        if (g == 0x1C)
        {
            getchar();
            if (!strcmp(name[selected], "../                           "))
            {
                *strrchr(cd, '/')       = 0;
                *(strrchr(cd, '/') + 1) = 0;
                return true;
            }
            else if (!strcmp(name[selected], "./                            "))
            {
                return true;
            }
            else if (strchr(name[selected], '/'))
            {
                *(strrchr(name[selected], '/') + 1) = 0;
                strcpy(cd + strlen(cd), name[selected]);
                return true;
            }
            else
            {
                char *prgm         = name[selected];
                *strchr(prgm, ' ') = 0;
                if (!strcmp(prgm + strlen(prgm) - 4, ".PRG") || !strcmp(prgm + strlen(prgm) - 4, ".SYS"))
                {
                    char *p_argv[1];
                    p_argv[0] = NULL;
                    char *p_envp[3];
                    p_envp[0] = "CD";
                    p_envp[1] = cd;
                    p_envp[2] = NULL;
                    strcpy(program, cd);
                    strcpy(program + strlen(program), prgm);
                    terminal_setcolor(0x0F);
                    terminal_clear();
                    uint32_t pid = exec_args(program, p_argv, p_envp);
                    waitpid(pid);
                    printf("Press a key to return to file manager...");
                    getkey();
                    unsigned int g;
                    while (g = getkey(), g == 0 || g > 128) yield();
                    return true;
                }
                else if (!strcmp(prgm + strlen(prgm) - 4, ".TXT") || !strcmp(prgm + strlen(prgm) - 4, ".RTF"))
                {
                    char *p_argv[2];
                    p_argv[0] = prgm;
                    p_argv[1] = NULL;
                    char *p_envp[3];
                    p_envp[0] = "CD";
                    p_envp[1] = cd;
                    p_envp[2] = NULL;
                    strcpy(program, cd);
                    strcpy(program + strlen(program), prgm);
                    terminal_setcolor(0x0F);
                    terminal_clear();
                    uint32_t pid = exec_args("A:/BIN/EDIT.PRG", p_argv, p_envp);
                    waitpid(pid);
                    printf("Press a key to return to file manager...");
                    getkey();
                    unsigned int g;
                    while (g = getkey(), g == 0 || g > 128) yield();
                    return true;
                }
                else
                {
                    prgm[strlen(prgm)] = ' ';
                }
            }
        }
        if (g == 0x01)
        {
            terminal_setcolor(0x0F);
            terminal_clear();
            exit(0);
        }
        if (g == 0x3B)
        {
            drawrect(15, 5, 50, 16, 0x0F);
            drawrect(17, 6, 46, 14, 0xF0);
            terminal_goto(24, 7);
            printf("Use the cursor to select a disk:");
            numdisks = 0;

            for (uint8_t i = 0; i < 8; i++)
            {
                testdisk[0] = 65 + i;
                FILE *fp    = openfile(testdisk, "r");
                if (fp->valid)
                {
                    disks[numdisks] = i;
                    numdisks++;
                }
            }
            while (1)
            {
                if (diskselector()) break;
            }
            return true;
        }

        if (g == 0x3C)
        {
            drawrect(27, 9, 25, 7, 0x0F);
            drawrect(29, 10, 21, 5, 0xF0);
            strcpy(program, cd);
            strcpy(program + strlen(program), name[selected]);
            char *a = strchr(program, ' ');
            if (a) *a = 0;
            FILE *fp = openfile(program, "r");
            terminal_goto(32, 11);
            terminal_setcolor(0xF0);
            if (fp->valid)
                printf("Size: %u bytes", fp->size);
            else
                printf("Could not open file.\n%s", program);
            terminal_goto(38, 13);
            terminal_setcolor(0x9F);
            printf(" OK ");
            g = 0;
            while (g != 0x1C) g = getkey();
            resetselection = false;
            return true;
        }

        if (g == 0x3F)
        {
            drawrect(27, 9, 25, 8, 0x0F);
            drawrect(29, 10, 21, 6, 0xF0);
            strcpy(program, cd);
            strcpy(program + strlen(program), name[selected]);
            char *a = strchr(program, ' ');
            if (a) *a = 0;
            terminal_goto(33, 11);
            terminal_setcolor(0xF0);
            printf("Are you sure?");
            terminal_goto(30, 12);
            printf("Press Esc to cancel");
            terminal_setcolor(0x9F);
            terminal_goto(35, 14);
            printf(" Delete ");
            g = 0;
            while (g != 0x1C && g != 1) g = getkey();
            if (g == 0x1C)
            {
                deletefile(program);
            }
            resetselection = true;
            return true;
        }
    }
}

void gui()
{
    terminal_setcolor(0x40);
    terminal_clear();
    terminal_setcolor(0x70);
    terminal_writestring("                             TritiumOS File Browser                             ");
    terminal_goto(0, 24);
    terminal_writestring(" Esc = exit | F1 = Change disk | F2 = Show file size | F5 = Delete File        ");
    terminal_putentryat(' ', 0x70, 79, 24);
    drawrect(20, 2, 40, 21, 0x0F);
    terminal_setcolor(0xF0);
    drawrect(22, 3, 36, 3, 0xF0);
    terminal_goto(23, 4);
    printf("Use the cursor to select a program");
    drawrect(22, 7, 36, 15, 0xF0);
    while (1)
    {
        if (programselector()) return;
    }
}

void main(uint32_t argc, char **argv)
{
    // terminal_init();
    char *env_cd = getenv("CD");
    if (!env_cd)
    {
        strcpy(cd, "A:/");
    }
    else
    {
        strcpy(cd, env_cd);
    }
    getkey();
    for (;;)
    {
        count = 0;
        if (resetselection)
            selected = 0;
        else
            resetselection = true;
        min     = 0;
        currdir = openfile(cd, "r");
        memset(name, 0, sizeof(char) * 16 * 31);
        FILE *r = currdir;
        for (uint8_t i = 0; i < 16; i++)
        {
            memset(buf, 0, 31);
            r = finddir(currdir, buf, i);
            if (r->valid && buf[0])
            {
                memset(name[count], ' ', 30);
                memcpy(name[count], buf, strlen(buf));
                for (uint8_t j = 0; j < 30; j++)
                {
                    if (!name[count][j]) name[count][j] = ' ';
                }
                count++;
            }
        }
        gui();
    }
}
