#include "../include/dropboxServer.h"
#include "../include/dropboxUtil.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define GET_SYNC_DIR 4

// variaveis globais de controle do servidor
struct client clients[10];
int semaforo = 0;
char buffer[BUFFER_SIZE];


void* servidor(void* args) {

	struct arg_struct *arguments = (struct arg_struct *)args;
	int instruction_id;
	int clientLen = sizeof(arguments->clientAddr);

	//printf("DEBUG instrucao recebido em hex: %x\n", arguments->my_datagram.instruction);

	instruction_id = desassembly_client_inst(arguments->my_datagram.instruction);
	printf("Instruction id: %d\n", instruction_id);

	if (instruction_id == ESTABLISH_CONNECTION) {

		// verifica se usuario existe no sistema
		if ( is_first_connection(arguments->my_datagram.username) ){

			// get available client slot
			int i = 0;
			for(i=0; i<MAXUSERS; i++) {
				if ( strcmp(clients[i].userid, "") == 0 ){
					break;
				}
			}
			// se o ultimo estiver preenchido
			if ( (i == MAXUSERS-1) && (strcmp(clients[MAXUSERS-1].userid, "") != 0) ){
				assembly_server_inst(&arguments->my_datagram.instruction, TOO_MANY_USERS);
			}
			else{
				strcpy(clients[i].userid, arguments->my_datagram.username);
				clients[i].devices[0] = TRUE;
				save_clients();

				// CRIAR DIRETORIO USERNAME AQUI

				assembly_server_inst(&arguments->my_datagram.instruction, FIRST_TIME_USER);
			}
		}

		else {
			int device_index = log_device(arguments->my_datagram.username);
			int client_index = get_client_index(arguments->my_datagram.username);

			if ( device_index != ERROR) {
				if (device_index == 0)
					clients[client_index].logged_in = TRUE;
					save_clients();
				assembly_server_inst(&arguments->my_datagram.instruction, CONNECTED);
			}
			else {
				assembly_server_inst(&arguments->my_datagram.instruction, TOO_MANY_DEVICES);
			}
		}

		assembly_server_inst(&arguments->my_datagram.instruction, ACK);
		sendto(arguments->s, &arguments->my_datagram, sizeof(struct datagram), 0, (struct sockaddr *)&arguments->clientAddr, clientLen);

		// sair da thread
	}

	if (instruction_id == EXIT) {

		int client_index = get_client_index(arguments->my_datagram.username);

		if (log_off_device(arguments->my_datagram.username) == 0){ // se deslogar do device cujo index é 0
			clients[client_index].logged_in = FALSE;
		}

		save_clients();

		assembly_server_inst(&arguments->my_datagram.instruction, TERMINATE_CLIENT_EXECUTION);
		assembly_server_inst(&arguments->my_datagram.instruction, ACK);
		sendto(arguments->s, &arguments->my_datagram, sizeof(struct datagram), 0, (struct sockaddr *)&arguments->clientAddr, clientLen);

	}

	if (instruction_id == UPLOAD) {

		// receber os dados e salvar no arquivo

		assembly_server_inst(&arguments->my_datagram.instruction, FILE_RECEIVED);
		assembly_server_inst(&arguments->my_datagram.instruction, ACK);
		sendto(arguments->s, &arguments->my_datagram, sizeof(struct datagram), 0, (struct sockaddr *)&arguments->clientAddr, clientLen);
	}



	// ELSE, OUTRAS INSTRUCOES

}

int main(int argc, char *argv[]) {

  	struct  sockaddr_in peer;
	SOCKET  s;
	int port;
	int peerlen, n;

	//Pega paramentro
	if(argc < 2) {
		printf("Utilizar:\n");
		printf("dropBoxServer <port>");
		exit(1);
	}
	port = atoi(argv[1]);  // Porta

	// prepara servidor e carrega informacoes (persistencia)
	if ( init_server() == ERROR){
		printf("Erro ao preparar o servidor informacoes do servidor\n");
	}

	s = setup_server(port);
    // recebe datagrama
    struct datagram received_datagram;
    int rc;
    struct arg_struct *args = NULL;

    while(1) {

    	SOCKET clientSocket;
    	struct  sockaddr_in clientAddr;
    	unsigned int clientLen;
    	clientLen = sizeof(clientAddr);

    	pthread_t thread;
		rc = recvfrom(s, &received_datagram, sizeof(struct datagram), 0, (struct sockaddr *) &clientAddr,(socklen_t *)&clientLen);

		if (rc < 0) {
			printf("Erro ao receber datagrama\n");
			return ERROR;
		}

		args = (struct arg_struct*)malloc(sizeof *args);
		args->my_datagram = received_datagram;
		args->s = s;
		args->clientAddr = clientAddr;

		// comecar uma thread aqui
		if ( pthread_create(&thread, NULL, servidor, args) != 0 ) {
			printf("Erro na criação da thread\n");
		}


    }
}

int setup_server(int port) {

	struct  sockaddr_in peer;

	SOCKET s;

	int peerlen, n;

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("Falha na criacao do socket\n");
	    exit(1);
 	}

    // Define domínio, IP e porta a receber dados
	memset((void *) &peer,0,sizeof(struct sockaddr_in));
	peer.sin_family = AF_INET;
	peer.sin_addr.s_addr = htonl(INADDR_ANY); // Recebe de qualquer IP
	peer.sin_port = htons(port); // Recebe na porta especificada na linha de comando
	peerlen = sizeof(peer);

	// Associa socket com estrutura peer
	if(bind(s,(struct sockaddr *) &peer, peerlen)) {
	    printf("Erro no bind\n");
	    exit(1);
	}

    printf("Socket inicializado. Aguardando mensagens...\n\n");
    return s;
}

int init_server() {

	// carrega a lista de clientes
	if(access("clients.dat", F_OK ) != -1)
		load_clients();
	else{
		for(int i=0; i<MAXUSERS; i++){
			clients[i].devices[0] = 0;
			clients[i].devices[1] = 0;
			strcpy(clients[i].userid, "");
			clients[i].logged_in = 0;
			save_clients();
		}

	}
	// carrega informacoes dos clientes que foram salvos
}

    /*create_database_structure();
	printf("Criou database!\n\n");

	// ------------ TESTANDO GET_SYNC_DIR ----------

	while(1) {
		ssize_t teste;

		struct client cliente;

		printf("Esperando pacote!\n");

		teste = recvfrom(s, &cliente, 12500, 0, (struct sockaddr *) &peer,(socklen_t *)&peerlen);


		strcpy(buffer,"ACK");

		if(teste != -1)
			printf("Pacote recebido! %zd\n", teste);
		else{
			printf("Pacote não recebido!\n");
		}
		//sleep(1);
	    n = sendto(s,buffer,sizeof(buffer),0,(struct sockaddr *) &peer, peerlen);
		if(n < 0){
			printf("Erro no envio do ACK!\n");
		}

		if(cliente.command_id == GET_SYNC_DIR){
		printf("%s\n", cliente.userid);
			create_path(cliente.userid);

		}

	}

	// ------------ FIM TESTE ----------------------


    receive_file(s, (struct sockaddr *) &peer, peerlen);

	return 0;*/







/*int create_database_structure() {

	char dir_name[20] = "database";

	if(mkdir(dir_name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0)
		//printf("Pasta %s criada.\n", dir_name);

{}
	else{
		//printf("Pasta %s já existe.\n", dir_name);
		return ERROR;
	}
	return SUCCESS;

}*/

/*
void receive_file(int s, struct sockaddr* peer, int peerlen){
  	struct package pack = create_package(s, peer, peerlen);

  	FILE* file_complete;
  	ssize_t bytes_receive = 0;

  	char buffer[1250];

	char dir[100] = "sync_dir_";
	strcat(dir, pack.username);
	strcat(dir, "/");
  	strcat(dir, pack.command.filename);
	file_complete = fopen(dir, "w");

  	fwrite(pack.buffer, 1, sizeof(pack.buffer), file_complete);
  	fclose(file_complete);
}

int client_count(char *user){
    int x, cont = 0;

    while (semaforo == 1){

    }
    semaforo = 1;
    for (x = 0; x < 10; x++){
        if (strcmp(user,clientes[x].userid) == 0 && clientes[x].logged_int == 1)
            cont++;
    }

    printf("Cliente tem %d conexoes \n", cont);

    semaforo = 0;

    return cont;
}

void send_file2(int s, char* user, struct sockaddr * peer, int peerlen){

    char dir[200] = "sync_dir_";
    strcat(dir,user);
    strcat(dir,"/");

    char buffer[1250];
    ssize_t bytes_send;
    ssize_t bytes_read;
    FILE* file_complete;

    bytes_send = recvfrom(s, buffer, sizeof(buffer),0, (struct sockaddr *) &peer,(socklen_t *)&peerlen);
      if (bytes_send < 0) printf("Erro\n");
      strcat(dir,buffer);

    if ((file_complete = fopen(dir, "r")) == NULL) {
        printf("Erro \n");
        return;
    }

    while ((bytes_read = fread(buffer, 1,sizeof(buffer), file_complete)) > 0){
        if ((bytes_send = sendto(s, buffer, bytes_read, 0, (struct sockaddr *)&peer, peerlen)) < bytes_read) { // Se a quantidade de bytes enviados, não for igual a que a gente leu, erro

            printf("Erro\n");
            return;
        }
    }

    fclose(file_complete);
}

void create_path(char *user){
printf("%s\n", user);
	struct stat st = {0};
	char dir[50] = "database/sync_dir_";
	strcat(dir, user);

	if(mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0)
		printf("Pasta %s criada.\n", dir);
	else printf("Pasta %s já existe.\n",dir);

	return;
}

*/


/*********************************************
*	FUNÇÕES AUXILIARES DO SERVIDOR
**********************************************/

int desassembly_client_inst(int word) {
	int instruction = word & 0xffff0000;

	if ( (instruction & 0xf8000000) == 0x08000000 ) {
		return ESTABLISH_CONNECTION;
	}

	if ( (instruction & 0xf8000000) == 0x10000000 ) {
		return UPLOAD;
	}

	if ( (instruction & 0xf8000000) == 0x18000000 ) {
		return DOWNLOAD;
	}

	if ( (instruction & 0xf8000000) == 0x20000000 ) {
		return LIST_SERVER;
	}

	if ( (instruction & 0xf8000000) == 0x28000000 ) {
		return LIST_CLIENT;
	}

	if ( (instruction & 0xf8000000) == 0x30000000 ) {
		return GET_SYNC_DIR;
	}

	if ( (instruction & 0xf8000000) == 0x38000000 ) {
		return EXIT;
	}

	return ERROR;
}


int assembly_server_inst(int *instruction, int instruction_id) {

	if (instruction_id == ACK) {
		*(instruction) = *(instruction) | 0x00000001; // coloca o 1 no início
		return SUCCESS;
	}

	if (instruction_id == CLEAR_INSTRUCTION_BYTE) {
		*(instruction) = *(instruction) & 0x00000000; // coloca o 1 no início
		return SUCCESS;
	}

	if (instruction_id == CONNECTED) {
		*(instruction) = *(instruction) & 0xffff0000; // zera os LSBs
		*(instruction) = *(instruction) | 0x00000900; // add instrucao
		return SUCCESS;
	}

	if (instruction_id == FIRST_TIME_USER) {
		*(instruction) = *(instruction) & 0xffff0000; // zera os LSBs
		*(instruction) = *(instruction) | 0x00000a00; // add instrucao
		return SUCCESS;
	}

	if (instruction_id == TOO_MANY_DEVICES) {
		*(instruction) = *(instruction) & 0xffff0000; // zera os LSBs
		*(instruction) = *(instruction) | 0x00000b00; // add instrucao
		return SUCCESS;
	}

	if (instruction_id == TOO_MANY_USERS) {
		*(instruction) = *(instruction) & 0xffff0000; // zera os LSBs
		*(instruction) = *(instruction) | 0x00000c00; // add instrucao
		return SUCCESS;
	}

	if (instruction_id == TERMINATE_CLIENT_EXECUTION) {
		*(instruction) = *(instruction) & 0xffff0000; // zera os LSBs
		*(instruction) = *(instruction) | 0x00000d00; // add instrucao
		return SUCCESS;
	}

	if (instruction_id == FILE_RECEIVED) {
		*(instruction) = *(instruction) & 0xffff0000; // zera os LSBs
		*(instruction) = *(instruction) | 0x00000e00; // add instrucao
		return SUCCESS;
	}


	/*if (instruction_id == UPLOAD) {
		*(instruction) = *(instruction) & 0x3fffffff; // coloca o 0 no início e 0 em custom code bit
		// botar aqui a máscara específica
		return SUCCESS;
	}
	if (instruction_id == DOWNLOAD) {
		*(instruction) = *(instruction) & 0x3fffffff; // coloca o 0 no início e 0 em custom code bit
		// botar aqui a máscara específica
		return SUCCESS;
	}
	if (instruction_id == LIST_SERVER) {
		*(instruction) = *(instruction) & 0x3fffffff; // coloca o 0 no início e 0 em custom code bit
		// botar aqui a máscara específica
		return SUCCESS;
	}
	if (instruction_id == LIST_CLIENT) {
		*(instruction) = *(instruction) & 0x3fffffff; // coloca o 0 no início e 0 em custom code bit
		// botar aqui a máscara específica
		return SUCCESS;
	}
	if (instruction_id == GET_SYNC_DIR) {
		*(instruction) = *(instruction) & 0x3fffffff; // coloca o 0 no início e 0 em custom code bit
		// botar aqui a máscara específica
		return SUCCESS;
	}
	if (instruction_id == EXIT) {
		*(instruction) = *(instruction) & 0x3fffffff; // coloca o 0 no início e 0 em custom code bit
		// botar aqui a máscara específica
		return SUCCESS;
	}*/

	printf("Erro ao determinar funcao\n");
	return ERROR;
}

int has_too_many_devices(char username[]) {

	//printf("Em has_to_many_devices: Nome do usuario eh: %s\n", username);
	struct client c = get_client(username);

	for(int i=0; i<MAXDEVICES; i++) {
		if( c.devices[i] == 0){
			return FALSE;
		}
	}
	return TRUE;
}

// FUNÇÕES DE CONTROLE

struct client get_client(char username[]) {
	struct client c;
	for(int i=0; i<MAXUSERS; i++){
		if( strcmp(clients[i].userid, username) == 0 )
			c = clients[i];
	}
	return c;
}

int get_client_index(char username[]) {

	for(int i=0; i<MAXUSERS; i++){
		if( strcmp(clients[i].userid, username) == 0 )
			return i;
	}
	return ERROR;
}

int is_first_connection(char username[]) {

	if( get_client_index(username) == -1 )
		return TRUE;
	return FALSE;
}

int log_device(char username[]) {

	int index = get_client_index(username);

	for(int i=0; i<MAXDEVICES; i++) {
		if(clients[index].devices[i] == 0) {
			clients[index].devices[i] = 1;
			return index;
		}
	}
	return ERROR;
}

int log_off_device(char username[]){
	int index = get_client_index(username);

	for(int i=MAXDEVICES-1; i>=0; i--) {
		if(clients[index].devices[i] == 1){
			clients[index].devices[i] = 0;
			return index;
		}
	}
	return ERROR;
}

int save_clients() {
    FILE * out_clients;
    out_clients = fopen ("clients.dat", "w");
    if (out_clients == NULL) {
        return ERROR;
    }

    int i;
    for(i=0;i<MAXUSERS;i++) {
        fwrite (&clients[i], sizeof(struct client), 1, out_clients);
    }

    fclose(out_clients);

    if(fwrite == 0)
        return ERROR;

    return SUCCESS;
}

int load_clients() {
    FILE * in_clients;

    in_clients = fopen ("clients.dat", "r");
    if (in_clients == NULL){
        return ERROR;
    }

    int i;
    for(i=0;i<MAXUSERS;i++) {
        fread(&clients[i], sizeof(struct client), 1, in_clients);
    }

    fclose(in_clients);

    return SUCCESS;
}
