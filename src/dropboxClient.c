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


int interface(){

	char line[100];
	char file_path[100];	// usado em upload e download
	char file_name[100]; 	// usado em download e upload
	char *command;

	while(1) {
		scanf("%[^\n]", line);
		getchar();

		/*
		TRATAR A LINHA DE ENTRADA
		*/
		printf("line: %s\n", line);
		//strcat(line, '\0');

		command = strtok(line, " ");

		printf("command: %s\n", command);


		// PREPARA INSTRUCAO
		if(strcmp(command, "upload") == 0){

			char file_path[100];

			command = strtok(NULL, ""); // coloca o que sobrou de volta em line (bem estranho como strtok() funciona)

			char* start_name_pointer;
			if ((int*)(start_name_pointer = strrchr(command, '/')) == NULL) { // last occurrence of '/'
				strcpy(file_path, command);
				strcpy(file_name, command);
			} 

			else {
				strncpy(file_path, command, start_name_pointer - command+1);
				strcpy(file_name, start_name_pointer+1);
			}
			printf("Path: %s\n", file_path);
			printf("File: %s\n", file_name);
			assembly_client_inst(&my_datagram.instruction, UPLOAD);
		}

		else if(strcmp(command, "download") == 0){
			printf("entrou no download\n");
		}

		else if(strcmp(command, "list_server") == 0){

		}

		else if(strcmp(command, "list_client") == 0){

		}

		else if(strcmp(command, "get_sync_dir") == 0){

		}

		else if(strcmp(command, "exit") == 0){
			printf("entrou no exit da interface\n");
			assembly_client_inst(&my_datagram.instruction, EXIT);
		}

		// ENVIA DATAGRAMA COM INSTRUCAO PRO SERVIDOR
		do {
			// envia datagrama
			rc = sendto(socket_id, &my_datagram, sizeof(struct datagram), 0, (struct sockaddr *)&peer, peerlen);
			// recebe datagrama com ACK
			rc = recvfrom(socket_id, &my_datagram, sizeof(struct datagram),0,(struct sockaddr *) &peer,(socklen_t *) &peerlen);

		} while (rc < 0 || ((my_datagram.instruction & 0x00000001) ^ 0x00000001) );

		
		// TRATA INSTRUCAO QUE VOLTAR DO SERVIDOR

		if (desassembly_server_inst(my_datagram.instruction) == TERMINATE_CLIENT_EXECUTION){
			printf("CLIENTE TERMINANDO\n");
			break;
		}


		else if (desassembly_server_inst(my_datagram.instruction) == START_SENDING){
			printf("CLIENTE VAI ENVIAR ARQUIVO\n");
			send_file(file_name);
			break;
		}

	}
	return 0;
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
	if(login_server(host, port) == ERROR) {
		printf("[main] Erro ao estabelecer sessao em login_server\n");
		exit(1);
	}

	// tratamento do que volta do servidar na hora da conexão
	int instruction = my_datagram.instruction;

	//printf("DEBUG: %x\n", instruction);

	int instruction_id = desassembly_server_inst(instruction);
	//printf("DEBUG: %d\n", instruction_id);

	if (handle_server_connectivity_status(instruction_id) == ERROR) {
			printf("\nDEBUG terminando a main sem conseguir se conectar ao servidor...\n");
			return 0;
	}

	create_sync_dir();
	interface();

	printf("\nDEBUG terminando a main conseguindo se conectar ao servidor...\n");

	return 0; // teste, remover

}



int login_server(char *host, int port) {

	// Timeout de 1 segundo
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 200000;

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
		rc = recvfrom(socket_id, &my_datagram, sizeof(struct datagram),0, (struct sockaddr *) &peer, (socklen_t *) &peerlen);

	} while (rc < 0 || ((my_datagram.instruction & 0x00000001) ^ 0x00000001) ); // recebe algo e recebe o ACK do servidor

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

int send_file(char *filename) {
	FILE * file;
	//char dir[100] = "sync_dir_";
	//strcat(dir,user_name);
	//strcat(dir,"/");
	//strcat(dir,filename);

	fprintf(stderr,"DEBUG: Entrou na funcao send_file.\n");
	//fprintf(stderr,"filename: %s\n", dir);
	fprintf(stderr,"filename: %s\n", filename);

	//file = fopen(dir,"r");
	file = fopen(filename, "r");
    if (file == NULL){
        return ERROR;
    }
	
	fprintf(stderr,"DEBUG: Abriu arquivo na funcao send_file.\n");

	struct file_info fileinfo = {"teste","txt","ok",20};

	int rc;
	char buffer_ack[256];
	bzero(buffer_ack,256);

	do {
		// envia o file_info
		rc = sendto(socket_id, &fileinfo, sizeof(struct file_info), 0, (struct sockaddr *)&peer, peerlen);
		// recebe datagrama com ACK
		rc = recvfrom(socket_id, buffer_ack, 256,0,(struct sockaddr *) &peer,(socklen_t *) &peerlen);
	
		printf("%s\n", buffer_ack);

	} while (rc < 0 || strcmp(buffer_ack,"ACK_FILEINFO") ); // recebe algo e recebe o ACK do servidor


	fprintf(stderr,"DEBUG: ACK recebido funcao send_file.\n");
	

	//datagram = instruction, id, user, buffer
    struct datagram pkg = {0,1,"joao"};

    while(fread(pkg.buffer,sizeof(char),BUFFER_SIZE,file)) {
        //Envia o 'pkg.buffer'
        //Bloqueia até receber o ack
        //Quando receber o ack, continua no 'while'
    }

	fprintf(stderr,"BUFFER ARQUIVO: %s\n",pkg.buffer);

	do {
		// envia o file_info
		rc = sendto(socket_id, &pkg, sizeof(struct datagram), 0, (struct sockaddr *)&peer, peerlen);
		// recebe datagrama com ACK
		rc = recvfrom(socket_id, &pkg, sizeof(struct datagram), 0,(struct sockaddr *) &peer,(socklen_t *) &peerlen);


	} while (rc < 0 || pkg.id == 2 ); // recebe algo e recebe o ACK do servidor

    fclose(file);
    return SUCCESS;
}



/*********************************************
*	FUNÇÕES AUXILIARES
**********************************************/

int assembly_client_inst(int *instruction, int instruction_id) {

	*(instruction) = *(instruction) & 0x07ffffff;		// zera a parte que carrega as instrucoes

	printf("instruction_id: %d\n", instruction_id);

	if (instruction_id == ESTABLISH_CONNECTION) {
		*(instruction) = *(instruction) | 0x08000000; 	// coloca o 1 no início
		return SUCCESS;
	}
	if (instruction_id == UPLOAD) {
		*(instruction) = *(instruction) | 0x10000000; 	//
		// botar aqui a máscara específica
		return SUCCESS;
	}
	if (instruction_id == DOWNLOAD) {
		*(instruction) = *(instruction) | 0x18000000; 	//
		// botar aqui a máscara específica
		return SUCCESS;
	}
	if (instruction_id == LIST_SERVER) {
		*(instruction) = *(instruction) | 0x20000000; 	//
		// botar aqui a máscara específica
		return SUCCESS;
	}
	if (instruction_id == LIST_CLIENT) {
		*(instruction) = *(instruction) | 0x28000000; 	//
		// botar aqui a máscara específica
		return SUCCESS;
	}
	if (instruction_id == GET_SYNC_DIR) {
		*(instruction) = *(instruction) | 0x30000000; 	//
		// botar aqui a máscara específica
		return SUCCESS;
	}
	if (instruction_id == EXIT) {
		*(instruction) = *(instruction) | 0x38000000; 	//
		return SUCCESS;
	}

	printf("Erro ao determinar funcao\n");
	return ERROR;
}

int desassembly_server_inst(int word) {

	// mascara = 0x0000f800

	if ( (word & 0x0000f800) == 0x00000800) {  // se for ESTABLISH_CONNECTION
		printf("entrou no establish\n");
		return desassembly_server_inst_status(word, ESTABLISH_CONNECTION);
	}

	if ( (word & 0x0000f800) == 0x00003800) {
		printf("entrou no terminate"); 
		return TERMINATE_CLIENT_EXECUTION;
	}

	if ( (word & 0x0000f800) == 0x00001000) {
		printf("entrou no upload no desassembly\n"); 
		return START_SENDING;
	}

	return ERROR;
}


int desassembly_server_inst_status(int word, int inst) {

	// mascara = 0x00000700

	// STATUS PARA ESTABLISH_CONNECTION
	if( inst == ESTABLISH_CONNECTION) {
		printf("entrou no status");
		if ( (word & 0x00000700) == 0x00000100) {
			return CONNECTED;
		}
		if ( (word & 0x00000700) == 0x00000200) {
			return FIRST_TIME_USER;
		}
		if ( (word & 0x00000700) == 0x00000300) {
			return TOO_MANY_DEVICES;
		}
		if ( (word & 0x00000700) == 0x00000400) {
			return TOO_MANY_USERS;
		}
	}

	// STATUS PARA OUTRAS INSTRUÇÕES

	return ERROR;
}

int handle_server_connectivity_status(int instruction_id){

	if( instruction_id == FIRST_TIME_USER) {
		printf("first time user\n");
		return SUCCESS;
	}

	if( instruction_id == CONNECTED) {
		//create_sync_dir();
		printf("connected\n");
		return SUCCESS;
	}

	if( instruction_id == TOO_MANY_DEVICES) {
		printf("too many devices\n");
		return ERROR;
	}

	if( instruction_id == TOO_MANY_USERS) {
		printf("too many users\n");
		return ERROR;
	}

	printf("Server instruction nao existe");
	return ERROR;

}

