#include "../include/dropboxUtil.h"
#include <stdio.h>
#include <string.h>

#define ERROR -1
#define UPLOAD 0
#define DOWNLOAD 1
#define LIST_SERVER 2
#define LIST_CLIENT 3
#define GET_SYNC_DIR 4
#define EXIT 5

int getCommand_id(char *command){

  if (strcmp(command, "upload") == 0){
    printf("DEBUG: COMMAND = upload\n");
    return UPLOAD;
  }

  else if (strcmp(command, "download") == 0)
    return DOWNLOAD;

  else if (strcmp(command, "list_server") == 0)
    return LIST_SERVER;

  else if (strcmp(command, "list_client") == 0)
    return LIST_CLIENT;

  else if (strcmp(command, "get_sync_dir") == 0)
    return GET_SYNC_DIR;

  else if (strcmp(command, "exit") == 0)
    return EXIT;

  else
    return ERROR;

}

void populate_instruction(char line[], struct instruction *inst) {
  
  char command[50], path[50], filename[40];

  // quebrar a linha em partes
  


  //inst->command_id = getCommand_id(command);
  //strcmp(inst->path, path);
  //strcmp(inst->filename, filename);
}