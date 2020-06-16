#include <stdio.h>
#include <string.h>

__asm__("jmp main");

static inline void syscall(unsigned int syscall_num) {
	asm volatile("mov %0,%%eax;int $0x80"::"r"(syscall_num));
}

void yield() {
	syscall(6);
}

#define NUM_COMMANDS 7

char *commands[NUM_COMMANDS] = {
	"help",
	"exit",
	"room",
	"north/n",
	"south/s",
	"east/e",
	"west/w"
};

char *helptext[NUM_COMMANDS] = {
	"Shows help",
	"Exits program",
	"Show info about the room",
	"Go North",
	"Go South",
	"Go East",
	"Go West"
};

char *cr_texts[3][3] = {
	{
		"You are in a room with no doors because the programmer was too lazy to put them in.",
		"Error",
		"Error",
	},
	{
		"Error",
		"Error",
		"Error",
	},
	{
		"Error",
		"Error",
		"Error",
	}
};

uint8_t cmd[128];
uint8_t index = 0;
char *current_room_text;
uint8_t cr_y = 0;
uint8_t cr_x = 0;

uint8_t run() {
	printf("> ");
	for(;;) {
		char c = 0;
		while (!c) {
			c = getchar();
			if (!c) {
				if (getkey()==14&&index) {
					terminal_backup();
					putchar(' ');
					terminal_backup();
					index--;
					cmd[index]=0;
				}
			}
			yield();
		}
		if (c=='\n') {
			printf("\n");
			break;
		}
		if (index!=127) {
			putchar(c);
			cmd[index] = c;
			cmd[index+1] = 0;
			index++;
		}
	}
	
	if (strcmp(cmd,"room")) {
		printf("%s\n",current_room_text);
	}
	
	if (strcmp(cmd,"help")) {
		for (uint8_t i = 0; i < NUM_COMMANDS; i++) {
			printf("%s - %s\n",commands[i],helptext[i]);
		}
	}
	
	if (strcmp(cmd,"exit"))
		return 0;
	else {
		index = 0;
		memset(cmd,0,128);
		return 1;
	}
}

_Noreturn void main() {
	terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK));
	printf("Welcome to Text Adventure!\n");
	printf("This game is distributed with TritiumOS\n");
	printf("under the MIT license.\n");
	printf("https://raw.githubusercontent.com/foliagecanine/tritium-os/master/LICENSE\n\n");
	printf("Enter a command or type \"help\" for help\n\n");
	current_room_text = cr_texts[cr_x][cr_y];
	while(run())
		;
	exit(0);
}