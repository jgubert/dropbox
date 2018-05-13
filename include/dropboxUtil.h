#include <stdio.h>

struct instruction {
	int command_id;
	char path[70];
	char filename[20];
}

struct package {
	char username[20];
	struct instruction command;
	char buffer[1250];
};


int getCommand(char *command);
