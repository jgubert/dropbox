#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define ERROR -1
#define SUCCESS 1
#define TRUE 1
#define FALSE 0

#define BUFFER_SIZE 1250
#define MAXUSERS 10
#define MAXDEVICES 2
#define MAXNAMES 8
#define MAXNAME 8
#define MAXFILES 10
#define USER_NAME_MAX_LENGTH 32

// client instructions
#define UPLOAD 0
#define DOWNLOAD 1
#define LIST_SERVER 2
#define LIST_CLIENT 3
#define GET_SYNC_DIR 4
#define EXIT 5

// custom instructions id
#define CLEAR_INSTRUCTION_BYTE 20
#define ACK 21
#define ESTABLISH_CONNECTION 22
#define FIRST_TIME_USER 23
#define CONNECTED 24
#define TOO_MANY_DEVICES 25
#define TOO_MANY_USERS 26
#define TERMINATE_CLIENT_EXECUTION 27
#define START_SENDING 28
#define START_DOWNLOAD 29

#define SOCKET int

struct file_info{
  char name[MAXNAME];
  char extension[MAXNAME];
  char last_modified[MAXNAME];
  int size;
};

struct datagram {
  int instruction;
  int id;
  char username[USER_NAME_MAX_LENGTH];
  struct file_info file;
  char buffer[BUFFER_SIZE];
};

struct arg_struct {
  struct datagram my_datagram;
  int s;
  struct sockaddr_in clientAddr;
};

struct instruction {
	int command_id;
	char path[70];
	char filename[USER_NAME_MAX_LENGTH];
};

struct package {
	char username[USER_NAME_MAX_LENGTH];
	struct instruction command;
	char buffer[BUFFER_SIZE];
};

struct client{
  int devices[MAXDEVICES];
  char userid[MAXNAMES];
  struct file_info info[MAXFILES];
  int logged_in;
	//atributos adicionados
  //int command_id;
  //char buffer[1250];
};

int getCommand_id(char *command);
void populate_instruction(char line[], struct instruction *inst);
void list_server(char* user, int socket_id);
