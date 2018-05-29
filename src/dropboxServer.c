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

// --- FUNÇÕES AUXILIARES ---

int create_database_structure();
void create_path(char *user);



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
				create_database_structure();
				create_path(clients[i].userid);

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
		printf("Servidor entrou no if do UPLOAD!\n");

		// FILE_RECEIVED é temporario.. primeiro envia o ACK para começar o envio do arquivo.
		assembly_server_inst(&arguments->my_datagram.instruction, START_SENDING);
		assembly_server_inst(&arguments->my_datagram.instruction, ACK);
		sendto(arguments->s, &arguments->my_datagram, sizeof(struct datagram), 0, (struct sockaddr *)&arguments->clientAddr, clientLen);

		receive_file(arguments->my_datagram.file.name,arguments->s,(struct sockaddr*)&arguments->clientAddr, clientLen,arguments->my_datagram.username);

		// receber os dados e salvar no arquivo

		assembly_server_inst(&arguments->my_datagram.instruction, ACK);
		sendto(arguments->s, &arguments->my_datagram, sizeof(struct datagram), 0, (struct sockaddr *)&arguments->clientAddr, clientLen);
	}

	if (instruction_id == DOWNLOAD){
		printf("Servidor entrou no if do DOWNLOAD!\n");

		assembly_server_inst(&arguments->my_datagram.instruction, START_DOWNLOAD);
		assembly_server_inst(&arguments->my_datagram.instruction, ACK);
		sendto(arguments->s, &arguments->my_datagram, sizeof(struct datagram), 0, (struct sockaddr *)&arguments->clientAddr, clientLen);

		send_file(arguments->s,(struct sockaddr*)&arguments->clientAddr, clientLen,arguments->my_datagram.username);

		assembly_server_inst(&arguments->my_datagram.instruction, ACK);
		sendto(arguments->s, &arguments->my_datagram, sizeof(struct datagram), 0, (struct sockaddr *)&arguments->clientAddr, clientLen);
	}

	// ELSE, OUTRAS INSTRUCOES
	printf("Acabou a thread: %d\n", instruction_id);

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

	SOCKET clientSocket;
   	struct  sockaddr_in clientAddr;
    unsigned int clientLen;
    clientLen = sizeof(clientAddr);

    while(1) {

    	pthread_t thread;
		rc = recvfrom(s, &received_datagram, sizeof(struct datagram), 0, (struct sockaddr *) &clientAddr,(socklen_t *)&clientLen);

		if (rc < 0) {
			printf("Erro ao receber datagrama\n");
			return ERROR;
		}
		else printf("Servidor recebeu um datagram\n");

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
}

int create_database_structure() {

	char dir_name[20] = "database";

	if(mkdir(dir_name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0)
		//printf("Pasta %s criada.\n", dir_name);

{}
	else{
		//printf("Pasta %s já existe.\n", dir_name);
		return ERROR;
	}
	return SUCCESS;

}

void receive_file(char *file, int s, struct sockaddr* peer, int peerlen, char *userid){
	int rc;
	struct file_info fileinfo;
	char buffer_ack[15];
	bzero(buffer_ack,15);
	struct datagram pkg;

	char dir[100] = "database/sync_dir_";

	strcat(dir, userid);
	strcat(dir,"/");

	printf("DEBUG: Entrou na receive_files\n");

	rc = recvfrom(s, &fileinfo, sizeof(struct file_info), 0, (struct sockaddr*) peer, (socklen_t *) &peerlen);
	printf("DEBUG FILE_INFO\nName: %s\nExt: %s\nLast Modified: %s\nSize: %d\n",fileinfo.name,fileinfo.extension,fileinfo.last_modified,fileinfo.size);
	strcpy(buffer_ack, "ACK_FILEINFO");
	fprintf(stderr,"buffer_ack: %s\n",buffer_ack);
	rc = sendto(s, buffer_ack, 15, 0, (struct sockaddr*) peer, peerlen);

	fprintf(stderr,"DEBUG: ACK enviado receive_files\n");

	strcat(dir,fileinfo.name);
	//strcat(dir,".");
	strcat(dir,fileinfo.extension);

	fprintf(stderr,"DEBUG: %s\n", dir);

	FILE * write_file;
    write_file = fopen (dir, "w");
    if (write_file == NULL) {
        printf("ERRO AO ABRIR O ARQUIVO PARA ESCRITA!\n");
    }

	rc = recvfrom(s, &pkg, sizeof(struct datagram), 0, (struct sockaddr*) peer, (socklen_t *) &peerlen);
	pkg.id = 2;
	rc = sendto(s, &pkg, sizeof(struct datagram), 0, (struct sockaddr*) peer, peerlen);

	fwrite(pkg.buffer, fileinfo.size, 1, write_file);
	fclose(write_file);
}


int send_file(int s, struct sockaddr* peer, int peerlen, char* userid){
	FILE * file;
	int rc;
	struct file_info fileinfo;

	char dir[100] = "database/sync_dir_";
	strcat(dir, userid);
	strcat(dir,"/");

	printf("DEBUG: Entrou na send_files\n");

	rc = recvfrom(s, &fileinfo, sizeof(struct file_info), 0, (struct sockaddr*) peer, (socklen_t *) &peerlen);
	printf("DEBUG FILE_INFO\nName: %s\nExt: %s\nLast Modified: %s\nSize: %d\n",fileinfo.name,fileinfo.extension,fileinfo.last_modified,fileinfo.size);

	strcat(dir,fileinfo.name);
	strcat(dir,fileinfo.extension);
	file = fopen(dir, "r");
	if (file == NULL){
			return ERROR;
	}
	fprintf(stderr,"DEBUG: abriu arquivo send_file\n");

	struct stat st;
	stat(dir, &st);
	int length = st.st_size;
	//fileinfo.size = length;
	fileinfo.size = length;

	printf("Size: %d\n", length);

	rc = sendto(s, &fileinfo, sizeof(struct file_info), 0, (struct sockaddr*) peer, peerlen);

	fprintf(stderr,"DEBUG: ACK enviado send_files\n");


	struct datagram pkg = {1,1};
	strcpy(pkg.username, userid);

	while(fread(pkg.buffer,sizeof(char),BUFFER_SIZE,file)) {
			//Envia o 'pkg.buffer'
			//Bloqueia até receber o ack
			//Quando receber o ack, continua no 'while'
	}

	fprintf(stderr,"BUFFER ARQUIVO: %s\n",pkg.buffer);

	do {
	// envia o file_info
		fprintf(stderr, "Entrou no while\n");
		rc = sendto(s, &pkg, sizeof(struct datagram), 0, (struct sockaddr*) peer, peerlen);
	// recebe datagrama com ACK
		rc = recvfrom(s, &pkg, sizeof(struct datagram), 0, (struct sockaddr*) peer, (socklen_t *) &peerlen);
		printf("PKG.ID: %d\n\n", pkg.id);


	} while (rc < 0 || pkg.id != 2 ); // recebe algo e recebe o ACK do servidor

	fclose(file);

	fprintf(stderr,"DEBUG: Saindo da função send_file\n");
	fprintf(stderr,"\n");



}

/*********************************************
*	FUNÇÕES AUXILIARES DO SERVIDOR
**********************************************/

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
		printf("desassembly_client_inst, ENTROU NO EXIT");
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

	// LOGIN_SERVER
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

	// EXIT

	if (instruction_id == TERMINATE_CLIENT_EXECUTION) {
		*(instruction) = *(instruction) & 0xffff0000; // zera os LSBs
		*(instruction) = *(instruction) | 0x00003800; // add instrucao
		return SUCCESS;
	}

	// UPLOAD

	if (instruction_id == START_SENDING) {
		*(instruction) = *(instruction) & 0xffff0000; // zera os LSBs
		*(instruction) = *(instruction) | 0x00001100; // add instrucao + status
		return SUCCESS;
	}

	// DOWNLOAD
	if (instruction_id == START_DOWNLOAD) {
		*(instruction) = *(instruction) & 0xffff0000; // zera os LSBs
		*(instruction) = *(instruction) | 0x00001800; // add instrucao + status
		return SUCCESS;
	}


	printf("Erro ao determinar assembly_server_inst()\n");
	return ERROR;
}

int has_too_many_devices(char username[]) {

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
