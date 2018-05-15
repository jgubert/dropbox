#include <stdio.h>

#define ERROR -1
#define SUCCESS 1

struct instruction {
	int command_id;
	char path[70];
	char filename[20];
};

struct package {
	char username[20];
	struct instruction command;
	char buffer[1250];
};


int getCommand_id(char *command);
void populate_instruction(char line[], struct instruction *inst);
void list_server(char* user, int socket_id);
