#include "../include/dropboxUtil.h"
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

void list_server(char* user, int socket_id) {

     DIR *dir;
     struct dirent *ent;
     ssize_t bytes_send;

     char user_dir[100] = "sync_dir_";
     strcat(user_dir, user);
     strcat(user_dir, "/");

     char userFiles[1250] = "";

     if ((dir = opendir (user_dir)) != NULL) {
         printf("Lendo diretorio do cliente\n");

         while ((ent = readdir (dir)) != NULL) {
            // estrutura do struct dirent -> char   d_name[]   Name of entry
             if((strcmp(ent->d_name,".") != 0 && strcmp(ent->d_name,"..") != 0)) {
                 strcat(userFiles, ent->d_name);
                 strcat(userFiles, "\n");
             }
         }

         if (strcmp(userFiles, "") == 0) {
             strcat(userFiles, "Nao existem arquivos\n");
         }
         strcat(userFiles, "\0");

         closedir (dir);

         if ((bytes_send = send(socket_id, userFiles, 1250, 0)) < 0) {
             printf("Erro\n");
             return;
         }

     } else {
         printf("Erro\n");
     }
     printf("Sucesso\n");
 }

void populate_instruction(char line[], struct instruction *inst) {

  char *command, path[50], filename[40];
  int comm_type;

  // quebrar a linha em partes
  printf("Line: %s\n", line);


  command = strtok(line, " ");
  printf("Command: %s\n", command);

  comm_type = getCommand_id(command);
  printf("comm_type: %d\n", comm_type);

  //PREENCHE PATH E FILENAME DO UPLOAD
  switch (comm_type) {
    case UPLOAD:
      line = strtok(NULL, ""); // coloca o que sobrou de volta em line (bem estranho como strtok() funciona
      printf("Nova linha: %s\n", line);

      char* start_name_pointer = strrchr(line, '/'); // last occurrence of '/'

      printf("\n%ld\n", start_name_pointer-line);
      strncpy(path, line, start_name_pointer - line+1);

      printf("Path: %s\n", path);

      strcpy(filename, start_name_pointer+1);
      printf("Filename: %s\n", filename);

      break;
    case DOWNLOAD:
      line = strtok(NULL, ""); // coloca o que sobrou de volta em line (bem estranho como strtok() funciona
      printf("Nova linha: %s\n", line);

      strcpy(filename, line);
      printf("Filename: %s\n", filename);
      strcpy(path, "");
      printf("Path: %s\n", path);
      break;
    default:
      strcpy(path, "");
      strcpy(filename, "");
      printf("Filename: %s\n", filename);
      printf("Path: %s\n", path);
      break;
  }

  inst->command_id = comm_type;
  strcpy(inst->path, path);
  strcpy(inst->filename, filename);

  return;
}
