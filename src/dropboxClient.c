#include "../include/dropboxUtil.h"
#include "../include/dropboxClient.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/time.h>

#define MAX_COMMAND_SIZE 15
#define	MAX_FILE_NAME_SIZE 25
#define USER_NAME_SIZE 25


char user_name[USER_NAME_MAX_LENGTH];
int user_socket_id;
struct sockaddr_in peer;
SOCKET socket_id;
int peerlen, rc;
char buffer[BUFFER_SIZE];
char buffer_receiver[BUFFER_SIZE];


struct datagram my_datagram; // datagrama que será enviado


void interface(){

	char line[100];

	while(1) {
		scanf("%[^\n]", line);
		getchar();

		/*
		TRATAR A LINHA DE ENTRADA
		*/
		//printf("%d", strlen(line));
		printf("line: %s\n", line);
		//strcat(line, '\0');

		strcpy(command, line);

		printf("command: %s\n", command);

		if(strcmp(command, "upload") == 0){
			printf("entrou no upload");

		}

		else if(strcmp(command, "download") == 0){

		}

		else if(strcmp(command, "list_server") == 0){

		}

		else if(strcmp(command, "list_client") == 0){

		}

		else if(strcmp(command, "get_sync_dir") == 0){

		}

		else if(strcmp(command, exit) == 0){
			printf("entrou no exit da interface");
			assembly_client_inst(&my_datagram.instruction, EXIT);
		}


		do {
			// envia datagrama

			rc = sendto(socket_id, &my_datagram, sizeof(struct datagram), 0, (struct sockaddr *)&peer, peerlen);
			// recebe datagrama com ACK
			rc = recvfrom(socket_id, &my_datagram, sizeof(struct datagram),0,(struct sockaddr *) &peer,(socklen_t *) &peerlen);
	
		} while (rc < 0 || ((my_datagram.instruction & 0x00000001) ^ 0x00000001) );

	}
}

int main(int argc, char *argv[] ){

    int port;
    char * host;

    struct package pacote; // pacote que será enviado
    //struct datagram datagrama; // datagrama que será enviado

    if(argc < 4) {
    	printf("Utilizar:\n");
    	printf("dropBoxClient <user> <address> <port>\n");
    	exit(1);
    }

    // Leitura de parametros
	if(strlen(argv[1]) > USER_NAME_MAX_LENGTH) {
		printf("Erro: tamanho de usuario precisa ser menor que %d\n", USER_NAME_SIZE);
		exit(1);
	} else {
		strcpy(user_name, argv[1]);  // User
	}

	//strcpy(pacote.username, user_name);
	strcpy(my_datagram.username, user_name);

  	host = malloc(strlen(argv[2])); // Host
	strcpy(host, argv[2]);

  	port = atoi(argv[3]);   //Port

    // Estabelece sessao entre cliente e servidor e recebe instrucao do servidor com status da conexao
	if(login_server(host, port) == SUCCESS) {
		//TODO: implementar
	} else {
		printf("[main] Erro ao estabelecer sessao em login_server\n");
		exit(1);
	}

	// TRATAR O QUE VIER DO SERVIDOR AQUI!
	int instruction = my_datagram.instruction;

	printf("%x\n", instruction);

	int instruction_id = desassembly_server_inst(instruction);
	printf("%d\n", instruction_id);

	handle_server_instruction(instruction_id);

	printf("\nDEBUG terminando a main...\n");


	// entrar na interface AQUI
		// receber os comandos, preparar a instrucao e mandar pro servidor


	return 0; // teste, remover

	// chamar o get sync_dir aqui

	/*char line[100];

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
		printf("Recebido %s\n\n",buffer_receiver);

		sleep(10);

	}*/
}

int login_server(char *host, int port) {

	// Timeout de 1 segundo
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 100000;

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

	//Setando TimeOut
	if (setsockopt(socket_id, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
		perror("Error");
	}

	// prepara instrução
	assembly_client_inst(&my_datagram.instruction, ESTABLISH_CONNECTION);

	int rc;
	do {
		// envia datagrama
		rc = sendto(socket_id, &my_datagram, sizeof(struct datagram), 0, (struct sockaddr *)&peer, peerlen);
		// recebe datagrama com ACK
		rc = recvfrom(socket_id, &my_datagram, sizeof(struct datagram),0,(struct sockaddr *) &peer,(socklen_t *) &peerlen);

	} while (rc < 0 || ((my_datagram.instruction & 0x00000001) ^ 0x00000001) ); // recebe algo e recebe oACK do servidor




	return SUCCESS;
}




/*int get_sync_dir(){
	//TODO implementar
	//envia um pacote com as informações do cliente

	struct client cliente;

	memcpy(cliente.userid,user_name,sizeof(user_name));
	printf("%s\n", cliente.userid);
	cliente.logged_int = 1;
	cliente.command_id = 4; //4 É O CODIGO DA GET_SYNC_DIR


	//enviando "cliente" pro servidor criar a pasta do usuario
	while(1) {
		printf("Tentando enviar\n");
		int sendToResponse = sendto(socket_id, &cliente, 12500, 0, (struct sockaddr *)&peer, peerlen);
		printf("Sendto: %d\n", sendToResponse);
		if(sendToResponse < 0) {
			printf("Falha ao enviar\n");
		} else {
			printf("Enviado! ");
			printf("Numero de bytes: %d \n", sendToResponse);
		}
		// recebe um ACK

		rc = recvfrom(socket_id,buffer_receiver, sizeof(buffer_receiver),0,(struct sockaddr *) &peer,(socklen_t *) &peerlen);

		printf("RC: %d\n", rc);
		if(rc > 0) {
			printf("Recebido ACK na função get_sync_dir%s\n\n", buffer_receiver);
			return SUCCESS;
		} else {
			printf("Timeout no request!\n");
		}
	}

}*/


//FUNÇÃO PARA CRIAR A PASTA DO USUARIO
/*int create_sync_dir() {
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
}*/



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

/*void client_interface(struct package *pacote){

	switch (pacote->command.command_id) {
		case 0:
			fprintf(stderr, "Debug: Tentativa de ler arquivo!\n" );
			FILE * arquivo;
			char dir_name[100];
			char * dir_name2;

			bzero(dir_name,100);

			memset(&dir_name2,0,sizeof(dir_name2));

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

			arquivo = fopen(dir_name,"r");
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
}*/

/*********************************************
*	FUNÇÕES AUXILIARES
**********************************************/

int assembly_client_inst(int *instruction, int instruction_id) {

	*(instruction) = *(instruction) & 0x07ffffff;		// zera a parte que carrega as instrucoes

	if (instruction_id == ESTABLISH_CONNECTION) {
		*(instruction) = *(instruction) | 0x08000000; 	// coloca o 1 no início
		return SUCCESS;
	}
	if (instruction_id == UPLOAD) {
		*(instruction) = *(instruction) & 0x10000000; 	//
		// botar aqui a máscara específica
		return SUCCESS;
	}
	if (instruction_id == DOWNLOAD) {
		*(instruction) = *(instruction) & 0x18000000; 	//
		// botar aqui a máscara específica
		return SUCCESS;
	}
	if (instruction_id == LIST_SERVER) {
		*(instruction) = *(instruction) & 0x20000000; 	//
		// botar aqui a máscara específica
		return SUCCESS;
	}
	if (instruction_id == LIST_CLIENT) {
		*(instruction) = *(instruction) & 0x28000000; 	//
		// botar aqui a máscara específica
		return SUCCESS;
	}
	if (instruction_id == GET_SYNC_DIR) {
		*(instruction) = *(instruction) & 0x30000000; 	//
		// botar aqui a máscara específica
		return SUCCESS;
	}
	if (instruction_id == EXIT) {
		*(instruction) = *(instruction) & 0x38000000; 	//
		return SUCCESS;
	}

	printf("Erro ao determinar funcao\n");
	return ERROR;
}

int desassembly_server_inst(int word) {

	if ( (word & 0x0000ff00) == 0x00000900 ) {
		return CONNECTED;
	}
	if ( (word & 0x0000ff00) == 0x00000a00 ) {
		return FIRST_TIME_USER;
	}
	if ( (word & 0x0000ff00) == 0x00000b00 ) {
		return TOO_MANY_DEVICES;
	}
	if ( (word & 0x0000ff00) == 0x00000c00 ) {
		return TOO_MANY_USERS;
	}

	return ERROR;
}

int handle_server_instruction(int instruction_id){

	if( instruction_id == FIRST_TIME_USER) {
		printf("first time user\n");
		interface();
		return SUCCESS;
	}

	if( instruction_id == CONNECTED) {
		printf("connected\n");
		interface();
		return SUCCESS;
	}

	if( instruction_id == TOO_MANY_DEVICES) {
		printf("too many devices\n");
		return SUCCESS;
	}

	if( instruction_id == TOO_MANY_USERS) {
		printf("too many users\n");
		return SUCCESS;
	}

	printf("Server instruction nao existe");

}
