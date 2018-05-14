//#include "../include/dropboxClient.h"
#include "../include/dropboxUtil.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SOCKET	int
#define MAX_COMMAND_SIZE 15
#define	MAX_FILE_NAME_SIZE 25
#define USER_NAME_SIZE 25
#define BUFFER_SIZE 100

#define ERROR -1
#define SUCCESS 1

char user_name[USER_NAME_SIZE];
int user_socket_id;
// teste
struct sockaddr_in peer;
SOCKET socket_id;
int peerlen, rc;
char buffer[BUFFER_SIZE];
char buffer_receiver[BUFFER_SIZE];

int get_sync_dir() {
	//TODO implementar
	return SUCCESS;
}

int login_server(char *host, int port) {
    //struct sockaddr_in peer;
    //SOCKET socket_id;
    //int peerlen, rc;
    //char buffer[BUFFER_SIZE];

    // Cria o socket na familia AF_INET (Internet) e do tipo UDP (SOCK_DGRAM)
	if((socket_id = socket(AF_INET, SOCK_DGRAM,0)) < 0) {
		printf("Falha na criacao do socket\n");
		return ERROR;
 	}

    // Cria a estrutura com quem vai conversar
	peer.sin_family = AF_INET;
	peer.sin_port = htons(port);
	peer.sin_addr.s_addr = inet_addr(host);
	peerlen = sizeof(peer);

	printf("Criado socket #%d\n", socket_id);

	//Executa o comando get_sync_dir
	if(get_sync_dir() == SUCCESS) {
		printf("Diretorio sincronizado com sucesso\n");
	} else {
		printf("Error ao sincronizar diretorio\n");
		return ERROR;
	}

	// Envia pacotes Hello e aguarda resposta
	/*while(1) {
		strcpy(buffer,"Hello");
		sendto(socket_id, buffer, sizeof(buffer), 0, (struct sockaddr *)&peer, peerlen);
		printf("Enviado Hello\n");
		rc = recvfrom(socket_id,buffer,sizeof(buffer),0,(struct sockaddr *) &peer,(socklen_t *) &peerlen);
		printf("Recebido %s\n\n",&buffer);
		sleep(5);
	}*/

	return SUCCESS;

}

int main(int argc, char *argv[] ){
    int port;
    char * host;

    struct package pacote; // pacote que será enviado
    struct instruction command;

    if(argc < 4) {
    	printf("Utilizar:\n");
    	printf("dropBoxClient <user> <address> <port>\n");
    	exit(1);
    }

    // Leitura de parametros
	if(strlen(argv[1]) > USER_NAME_SIZE) {
		printf("Erro: tamanho de usuario precisa ser menor que %d\n", USER_NAME_SIZE);
		exit(1);
	} else {
		strcpy(user_name, argv[1]);  // User
	}

	strcpy(pacote.username, user_name);
	

    host = malloc(strlen(argv[2])); // Host
	strcpy(host, argv[2]);

    port = atoi(argv[3]);   //Port

    // Estabelece sessao entre cliente e servidor
	if(login_server(host, port) == SUCCESS) {
		//TODO: implementar
	} else {
		printf("[main] Erro ao estabelecer sessao em login_server\n");
		exit(1);
	}

	char line[100];

	while(1) {

		scanf("%[^\n]", line);
		getchar();

		populate_instruction(line, &command);
		fflush(stdin);

		// coloca a instrução no pacote
		printf("printa aqui\n");
		printf("%d\n", command.command_id);
		printf("%s\n", command.path);

		pacote.command = command;


		// envia o pacote
		sendto(socket_id, &pacote, sizeof(struct package), 0, (struct sockaddr *)&peer, peerlen);
		printf("Enviado Pacote\n");
		// recebe um ACK
		rc = recvfrom(socket_id,buffer_receiver, sizeof(buffer_receiver),0,(struct sockaddr *) &peer,(socklen_t *) &peerlen);
		printf("Recebido %s\n\n",&buffer_receiver);

		sleep(10);



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
