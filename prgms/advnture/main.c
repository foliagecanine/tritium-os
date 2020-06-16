#include <stdio.h>
#include <string.h>

__asm__("jmp main");

static inline void syscall(unsigned int syscall_num) {
	asm volatile("mov %0,%%eax;int $0x80"::"r"(syscall_num));
}

void yield() {
	syscall(6);
}

typedef struct {
	uint8_t x;
	uint8_t y;
	char *name;
	bool aquired;
} item;

#define MAX_INV_ITEMS 16

uint8_t inventory[MAX_INV_ITEMS];
uint8_t num_inv_items = 0;

item items[MAX_INV_ITEMS];
bool doors[12];

#define NUM_COMMANDS 12

char *commands[NUM_COMMANDS] = {
	"help",
	"exit",
	"room",
	"inv",
	"look {under/at} [object]",
	"take [object]",
	"place [object]",
	"open [container]",
	"north/n",
	"south/s",
	"east/e",
	"west/w"
};

char *helptext[NUM_COMMANDS] = {
	"Shows help",
	"Exits program",
	"Show info about the room",
	"Show items in your inventory",
	"Look under/at object specified",
	"Grab the specified object",
	"Place the specified object",
	"Open the specified container",
	"Go north",
	"Go south",
	"Go east",
	"Go west"
};

char *cr_texts[3][3] = {
	{
		"You are in a stone room with a red carpet and door to the north.",
		"You are in a stone room with a lit torch attached to the wall.\nThere is a door to the south and the north.",
		"You are in a stone room with a door to the east and the south.\nThere is a sign that says \"LOOK AT THE WALL.\"",
	},
	{
		"You are in a stone room with a chest in the center. There is a door to the west.",
		"Error: no room text",
		"You are in a stone room with a door to the west. It's rather dark in here.\nMaybe it's because there's no torch in the torch holder.",
	},
	{
		"Error: no room text",
		"Error: no room text",
		"Error: no room text",
	}
};

uint8_t cmd[128];
uint8_t index = 0;
char *current_room_text;
uint8_t cr_y = 0;
uint8_t cr_x = 0;

bool has(uint8_t iid) {
	for (uint8_t i = 0; i < MAX_INV_ITEMS; i++) {
		if (inventory[i]==iid)
			return true;
	}
	return false;
}

uint8_t give_item(uint8_t iid) {
	for (uint8_t i = 0; i < MAX_INV_ITEMS; i++) {
		if (inventory[i]==0xFF) {
			inventory[i]=iid;
			items[iid].aquired = true;
			num_inv_items++;
			return i;
		}
	}
	return 0xFF;
}

uint8_t remove_item(uint8_t iid) {
	for (uint8_t i = 0; i < MAX_INV_ITEMS; i++) {
		if (inventory[i]==iid) {
			inventory[i]=0xFF;
			num_inv_items--;
			return i;
		}
	}
}

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
	
	if (cr_x==2&&cr_y==2) {
		
	} else if (cr_x==2&&cr_y==1) {
		
	} else if (cr_x==2&&cr_y==0) {
		
	} else if (cr_x==1&&cr_y==2) {
		if (strcmp(cmd,"w")||strcmp(cmd,"west")) {
			printf("You walk west\n");
			cr_x--;
		}
		if ((strcmp(cmd,"s")||strcmp(cmd,"south"))&&doors[3]) {
			printf("You walk south\n");
			cr_y--;
		}
		if (strcmp(cmd,"place torch")&&has(2)) {
			printf("You place the torch in the torch holder.\n");
			printf("A door opens to the south.\n");
			items[2].x = cr_x;
			items[2].y = cr_y;
			cr_texts[cr_x][cr_y] = "You are in a stone room with a lit torch attached to the wall.\nThere is a door to the west.";
			remove_item(2);
			doors[3] = true;
		}
		if (strcmp(cmd,"take torch")&&items[2].x==cr_x&&items[2].y==cr_y) {
			printf("You take the torch. The door to the south closes.\n");
			doors[3]=false;
			give_item(2);
		}
	} else if (cr_x==1&&cr_y==1) {
		
	} else if (cr_x==1&&cr_y==0) {
		if (strcmp(cmd,"w")||strcmp(cmd,"west")) {
			printf("You walk west\n");
			cr_x--;
		}
		if (strcmp(cmd,"open chest")) {
			printf("You open the chest and find");
			if (has(1)) {
				printf(" nothing\n");
			} else {
				printf(" a blue key.\nYou take the key.\n");
				give_item(1);
			}
		}
	} else if (cr_x==0&&cr_y==2) {
		if (strcmp(cmd,"s")||strcmp(cmd,"south")) {
			printf("You walk south\n");
			cr_y--;
		}
		if (strcmp(cmd,"e")||strcmp(cmd,"east")) {
			if (doors[2]||has(1)) {
				if (has(1)) {
					printf("You insert the blue key.\nIt unlocks, but the key won't come out.\n");
					remove_item(1);
					doors[2]=true;
				}
				cr_x++;
				printf("You walk east\n");
			} else {
				printf("That door is locked.\n");
			}
		}
		if (strcmp(cmd,"look at wall")) {
			printf("You find tiny text on the wall that says \"Not here! The other room!\"\n");
		}
	} else if (cr_x==0&&cr_y==1) {
		if (strcmp(cmd,"n")||strcmp(cmd,"north")) {
			printf("You walk north\n");
			cr_y++;
		}
		if (strcmp(cmd,"s")||strcmp(cmd,"south")) {
			printf("You walk south\n");
			cr_y--;
		}
		if (strcmp(cmd,"place torch")&&has(2)) {
			printf("You place the torch in the torch holder.\n");
			items[2].x = cr_x;
			items[2].y = cr_y;
			cr_texts[cr_x][cr_y] = "You are in a stone room with a lit torch attached to the wall.\nThere is a door to the south and the north.";
			remove_item(2);
		}
		if (strcmp(cmd,"take torch")&&items[2].x==cr_x&&items[2].y==cr_y) {
			printf("You take the torch.\n");
			cr_texts[cr_x][cr_y] = "You are in a stone room with a door to the north and the south. It's rather dark in here.\nMaybe it's because there's no torch in the torch holder.",
			give_item(2);
		}
	} else if (cr_x==0&&cr_y==0) {
		if (strcmp(cmd,"n")||strcmp(cmd,"north")) {
			if (doors[1]||has(0)) {
				if (has(0)) {
					printf("You insert the red key.\nIt unlocks, but the key won't come out.\n");
					remove_item(0);
					doors[1]=true;
				}
				cr_y++;
				printf("You walk north\n");
			} else {
				printf("That door is locked.\n");
			}
		}
		if (strcmp(cmd,"e")&&doors[0]) {
			printf("You walk east\n");
			cr_x++;
		}
		if (strcmp(cmd,"look under carpet")&&!items[0].aquired) {
			printf("You look under the carpet and find a red key.\nYou take the key.\n");
			give_item(0);
		}
		if (strcmp(cmd,"look at carpet")) {
			if (!items[0].aquired)
				printf("A red carpet with a lump in the middle.\n");
			else
				printf("It's a red carpet.\n");
		}
		if (strcmp(cmd,"look at wall")) {
			cr_texts[0][0]=	"You are in a stone room with a red carpet, a door to the east, and a door to the north.",
			printf("You find a button on the wall and press it. A door to the east opens.\n");
			doors[0]=true;
		}
	}
	
	if (strcmp(cmd,"room")) {
		printf("%s\n",current_room_text);
	}
	
	if (strcmp(cmd,"hello")) {
		printf("You hear a voice out of nowhere: \"Hello there.\"\nYou can't find who said it.\n");
	}
	
	if (strcmp(cmd,"inv")) {
		printf("You have ");
		uint8_t items_found = 0;
		if (num_inv_items==0) {
			printf("no items");
		} else if (num_inv_items==1) {
			for (uint8_t i = 0; i < MAX_INV_ITEMS; i++) {
				if (has(i)) {
					printf("%s",items[i].name);
					break;
				}
			}
		} else {
			for (uint8_t i = 0; i < MAX_INV_ITEMS; i++) {
				if (has(i)) {
					if (items_found!=num_inv_items-1) {
						printf("%s, ",items[i].name);
						items_found++;
					} else {
						printf("and %s",items[i].name);
						break;
					}
				}
			}
		}
		printf(" in your inventory.\n");
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
	printf("This game is distributed with TritiumOS under the MIT license.\n");
	printf("https://raw.githubusercontent.com/foliagecanine/tritium-os/master/LICENSE\n\n");
	printf("Enter a command or type \"help\" for help\n\n");
	
	memset(inventory,0xFF,16);
	
	items[0].name = "a red key";
	items[1].name = "a blue key";
	items[2].name = "a torch";
	items[2].x = 0;
	items[2].y = 1;
	
	current_room_text = cr_texts[cr_x][cr_y];
	
	while(run())
		current_room_text = cr_texts[cr_x][cr_y];
	exit(0);
}