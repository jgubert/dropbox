//#include "../include/dropboxClient.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
	#include <winsock2.h>
#else
    #include <stdlib.h>
    #include <unistd.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#define SOCKET	int
#endif

#define MAX_COMMAND_SIZE 15
#define	MAX_FILE_NAME_SIZE 25
#define USER_NAME_SIZE 25
#define BUFFER_SIZE 100


int main( int argc, char *argv[] ){

    struct sockaddr_in peer;
    SOCKET s;
    int port, peerlen, rc;
    char ip[16], user[USER_NAME_SIZE];
    char buffer[BUFFER_SIZE];

#ifdef _WIN32
	 WSADATA wsaData;
  
	if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
		printf("Erro no startup do socket\n");
		exit(1);
	}
#endif

    if(argc < 4) {
    	printf("Utilizar:\n");
    	printf("dropBoxClient <user> <address> <port>");
    	exit(1);
    }

    // Pega parametros
    strcpy(user, argv[1]);  // User
    strcpy(ip, argv[2]);    // Ip
    port = atoi(argv[3]);   //Port

    // Cria o socket na familia AF_INET (Internet) e do tipo UDP (SOCK_DGRAM)
	if((s = socket(AF_INET, SOCK_DGRAM,0)) < 0) {
		printf("Falha na criacao do socket\n");
		exit(1);
 	}

    // Cria a estrutura com quem vai conversar 
	peer.sin_family = AF_INET;
	peer.sin_port = htons(port);
	peer.sin_addr.s_addr = inet_addr(ip); 
	peerlen = sizeof(peer);

	// Envia pacotes Hello e aguarda resposta
	while(1)
	{
		strcpy(buffer,"Hello");
		sendto(s, buffer, sizeof(buffer), 0, (struct sockaddr *)&peer, peerlen);
		printf("Enviado Hello\n");
#ifdef _WIN32
		rc = recvfrom(s,buffer,sizeof(buffer),0,(struct sockaddr *)&peer, &peerlen); 
		printf("Recebido %s\n\n",&buffer);
		Sleep(5000);
#else
		rc = recvfrom(s,buffer,sizeof(buffer),0,(struct sockaddr *) &peer,(socklen_t *) &peerlen); 
		printf("Recebido %s\n\n",&buffer);
		sleep(5);
#endif
	}


/*
	// interface
	char command[MAX_COMMAND_SIZE];

	// mais tarde, botar esse loop em uma função
	while (1) {
		scanf("%s", command);

		if (strcmp(command, "upload") == 0) {
			#ifdef DEBUG
			printf("fazer upload\n");
			#endifO
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
*/
}