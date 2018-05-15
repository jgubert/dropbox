#include <stdio.h>

#define ERROR -1
#define SUCCESS 1
#define MAXNAMES 8
#define MAXNAME 8
#define MAXFILES 10

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

struct file_info{
  char name[MAXNAME];
  char extension[MAXNAME];
  char last_modified[MAXNAME];
  int size;
};

struct client{
  int devices[2];
  char userid[MAXNAMES];
  struct file_info info[MAXFILES];
  int logged_int;
	//atributos adicionados
  int command_id;
  char buffer[1250];
};


int getCommand_id(char *command);
void populate_instruction(char line[], struct instruction *inst);
void list_server(char* user, int socket_id);
