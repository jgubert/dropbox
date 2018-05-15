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
#include <sys/stat.h>

#define SOCKET	int
#define MAX_COMMAND_SIZE 15
#define	MAX_FILE_NAME_SIZE 25
#define USER_NAME_SIZE 25
#define BUFFER_SIZE 12500

#define ERROR -1
#define SUCCESS 1

//estruturas que o usuario vai escolher
#define UPLOAD 0
#define DOWNLOAD 1
#define LIST_SERVER 2
#define LIST_CLIENT 3
#define GET_SYNC_DIR 4
#define EXIT 5


char user_name[USER_NAME_SIZE];
int user_socket_id;
// teste
struct sockaddr_in peer;
SOCKET socket_id;
int peerlen, rc;
char buffer[BUFFER_SIZE];
char buffer_receiver[BUFFER_SIZE];

int get_sync_dir();

int get_sync_dir(){
	//TODO implementar
	//envia um pacote com as informações do cliente

	struct client cliente;

	strcpy(cliente.userid,user_name);
	cliente.logged_int = 1;
	cliente.command_id = 4; //4 É O CODIGO DA GET_SYNC_DIR


	//enviando "cliente" pro servidor criar a pasta do usuario
		sendto(socket_id, &cliente, 12500, 0, (struct sockaddr *)&peer, peerlen);
		printf("Enviado cliente na função get_sync_dir\n");
		// recebe um ACK
		rc = recvfrom(socket_id,buffer_receiver, sizeof(buffer_receiver),0,(struct sockaddr *) &peer,(socklen_t *) &peerlen);
		printf("Recebido ACK na função get_sync_dir%s\n\n",&buffer_receiver);

	return SUCCESS;
}


//FUNÇÃO PARA CRIAR A PASTA DO USUARIO
int create_sync_dir() {
	//TODO implementar
	char dir_name[50] = "sync_dir_";

	strcat(dir_name,user_name);

	if(mkdir(dir_name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0)
		printf("Pasta %s criada.\n", dir_name);
	else{
		printf("Pasta %s já existe.\n", dir_name);
		return ERROR;
	}
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

	//Cria pasta do usuario
	if(create_sync_dir() == SUCCESS){
		printf("Diretorio criado com sucesso\n");
	}

	//Executa o comando get_sync_dir
	if(get_sync_dir() == SUCCESS) {
		printf("Diretorio sincronizado com sucesso\n");
	} else {
		printf("Error ao sincronizar diretorio\n");
		return ERROR;
	}

	return SUCCESS;

}

/* colocar na interface

void send_file(char *file){
	ssize_t bytes_read = 0; //quantidade de bytes lido ate entao
	ssize_t bytes_send = 0; //foi enviado ate entao
	ssize_t file_size = 0;

	int package_count = 0;
	strcpy(buffer, file);
	package_make();

	sendto(socket_id, &pacote, sizeof(struct package), 0, (struct sockaddr *)&peer, peerlen);
	printf("Enviado Pacote\n");

}

void package_make(char line){
	scanf("%[^\n]", line);
	getchar();

	populate_instruction(line, &pacote.command);
	fflush(stdin);
}  */

void client_interface(struct package *pacote){

	switch (pacote->command.command_id) {
		case 0:
			fprintf(stderr, "Debug: Tentativa de ler arquivo!\n" );
			FILE * arquivo;
			char dir_name[100];
			char * dir_name2;
			bzero(dir_name,100);

			memset(dir_name2,0,sizeof(dir_name2));

			printf("pos bzero: %s\n", dir_name);
			printf("dirname2: %s\n", dir_name2);
			
			strcat(dir_name2,"sync_dir_");
			printf("dirname2: %s\n", dir_name2);
			strcat(dir_name2,pacote->username);
			printf("dirname2: %s\n", dir_name2);
			strcat(dir_name2,"/");
			printf("dirname2: %s\n", dir_name2);
			strcat(dir_name2,pacote->command.path);
			strcat(dir_name2,pacote->command.filename);

			strcat(dir_name,dir_name2);

			//strcpy(dir_name,"sync_dir_joao/path/teste.txt");

			printf("Arquivo: %s\n", dir_name);

			//snprintf( dir_name2,  sizeof(dir_name), "%s", dir_name );

			//printf("Arquivo: %s\n", dir_name2);

			arquivo = fopen("sync_dir_joao/path/teste.txt","r");
			//arquivo = fopen(dir_name, "r");
		 	if (arquivo == NULL){
		 		fprintf(stderr, "Debug: não abriu arquivo\n" );
				exit(1);
			}

			fprintf(stderr, "Debug: ABRIU ESSE CARALHO\n" );

			fread(pacote->buffer,1,1250,arquivo);

			fclose(arquivo);
			break;

		default:
			break;

	}
}

int main(int argc, char *argv[] ){
    int port;
    char * host;

    struct package pacote; // pacote que será enviado

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

		populate_instruction(line, &pacote.command);
		fflush(stdin);

		//pacote.command = command;

		//FAZENDO UM TESTE NO CASO DO upload
		//ideia é abrir testar se arquivo que quer enviar existe,
		//caso exista coloca ele no buffer

		client_interface(&pacote);

		//send_file();

		// envia o pacote
		sendto(socket_id, &pacote, sizeof(struct package), 0, (struct sockaddr *)&peer, peerlen);
		printf("Enviado Pacote\n");
		// recebe um ACK
		rc = recvfrom(socket_id,buffer_receiver, sizeof(buffer_receiver),0,(struct sockaddr *) &peer,(socklen_t *) &peerlen);
		printf("Recebido %s\n\n",&buffer_receiver);

		sleep(10);

	}

}
