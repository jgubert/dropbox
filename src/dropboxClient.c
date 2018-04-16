#include "../include/dropboxClient.h"
#include <stdio.h>
#include <string.h>

#define MAX_COMMAND_SIZE 15
#define	MAX_FILE_NAME_SIZE 25

//int login_server(char *host, int port){return 0;}
//void sync_client(){}
//void send_file(char *file){}
//void get_file(char *file){}
//void delete_file(char *file){}
//void close_session(){}

/*char userid[10];
int address;
int port;*/

int main( int argc, char *argv[] ){

	/*
	strcpy(userid, argv[1]);
	address = atoi(argv[2]);
	port = atoi(argv[3]);

	printf("%s\n%d\n%d\n", userid, address, port);*/


	// interface
	char command[MAX_COMMAND_SIZE];

	// mais tarde, botar esse loop em uma função
	while (1) {
		scanf("%s", command);

		if (strcmp(command, "upload") == 0) {
			#ifdef DEBUG
			printf("fazer upload\n");
			#endif
			// fazer upload (continuar)

		}
		else if (strcmp(command, "download") == 0) {

			char file_name[MAX_FILE_NAME_SIZE];
			scanf("%s", file_name);

			#ifdef DEBUG
			printf("fazer download...\n");
			printf("command: %s\nfile name: %s\n\n", command, file_name);		
			#endif

			//fazer download (continuar)

		}
		else if (strcmp(command, "list_server") == 0) {
			
			#ifdef DEBUG
			printf("list_server\n");
			#endif

			// listar os arquivos do usuário salvos no servidor (continuar)
		}
		else if (strcmp(command, "list_client") == 0) {

			#ifdef DEBUG
			printf("list_client\n");
			#endif

			// listar arquivos salvos no diretorio "sync_dir_<nomeusuário>" (continuar)

		}
		else if (strcmp(command, "get_sync_dir") == 0) {

			#ifdef DEBUG
			printf("get_sync_dir\n");
			#endif

			//Cria o diretório sync_dir_<nomeusuário no /home do usuario (continuar)

		}
		else if (strcmp(command, "exit") == 0) {

			#ifdef DEBUG
			printf("exit\n");
			#endif

			// fecha a sessão com o usuário (continuar)

			return 0;
		}
		else {
			printf("erro\n");
			return 1;
		}

	}
	return 0;
}