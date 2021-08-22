#include <stdio.h>
#include <ipc.h>

int main() {
	if (register_ipc_port(0x6000)) {
		printf("Receiver active. Waiting for data.\n");
		waitipc(0x6000);
		printf("Notified. Now receiving...\n");
		size_t size = receive_ipc_size(0x6000);
		if (size != -1) {
			char *buffer = malloc(size);
			receive_ipc(0x6000, buffer);
			printf("Received message: %s\n", buffer);
		} else {
			printf("Failed?!\n");
		}
	} else {
		printf("Sender active. Sending data\n");
		char *message = "Hello, is this thing on?";
		transfer_ipc(0x6000, message, strlen(message)+1);
	}
	return 0;
}
