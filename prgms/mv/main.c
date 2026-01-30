#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: mv <source> <destination>\n");
        return 1;
    }

    const char *source = argv[1];
    const char *destination = argv[2];

    if (movefile(source, destination) != 0) {
        printf("Error: Could not move file from %s to %s\n", source, destination);
        return 1;
    }

    return 0;
}