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
int setup_server_TCP(int port);

void* backup1(void* args){

}

void* backup2(void* args){

}

void* listenFE(void* args){

	struct arg_portas *arguments = (struct arg_portas *)args;

	int sockfd, newsockfd, n;
	socklen_t clilen;
	char buffer[256];
	struct sockaddr_in addr_serv, addr_cli;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
			printf("ERROR opening socket");


			// seta informacoes IP/Porta locais
			addr_cli.sin_family = AF_INET;
			addr_cli.sin_addr.s_addr = htonl(INADDR_ANY);
			addr_cli.sin_port = htons(arguments->portaCli); //porta cliente

			// associa configuracoes locais com socket
			if (bind(sockfd, (struct sockaddr *) &addr_serv, sizeof(addr_serv)) < 0)
				printf("ERROR on binding");

		  // seta informacoes IP/Porta do servidor remoto
		  addr_serv.sin_family = AF_INET;
		  addr_serv.sin_addr.s_addr = inet_addr(&arguments->IPServ);
		  addr_serv.sin_port = htons(arguments->portaServ);
			bzero(&(addr_serv.sin_zero), 8);


		listen(sockfd, 8);

		clilen = sizeof(struct sockaddr_in);
	if ((newsockfd = accept(sockfd, (struct sockaddr *) &addr_cli, &clilen)) == -1)
		printf("ERROR on accept");

	bzero(buffer, 256);

	/* read from the socket */
	n = read(newsockfd, buffer, 256);
	if (n < 0)
		printf("ERROR reading from socket");
	printf("Here is the message: %s\n", buffer);

	/* write in the socket */
	n = write(newsockfd,buffer, 256);
	if (n < 0)
		printf("ERROR writing to socket");

	close(newsockfd);
	close(sockfd);


}

void* servidor(void* args) {

	struct arg_struct *arguments = (struct arg_struct *)args;
	int instruction_id;
	int clientLen = sizeof(arguments->clientAddr);

	//printf("DEBUG instrucao recebido em hex: %x\n", arguments->my_datagram.instruction);

	instruction_id = desassembly_client_inst(arguments->my_datagram.instruction);
	//printf("Instruction id: %d\n", instruction_id);

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
		//printf("Servidor entrou no if do UPLOAD!\n");

		// FILE_RECEIVED é temporario.. primeiro envia o ACK para começar o envio do arquivo.
		assembly_server_inst(&arguments->my_datagram.instruction, START_SENDING);
		assembly_server_inst(&arguments->my_datagram.instruction, ACK);
		sendto(arguments->s, &arguments->my_datagram, sizeof(struct datagram), 0, (struct sockaddr *)&arguments->clientAddr, clientLen);

		receive_file(arguments->my_datagram.file.name,arguments->s,(struct sockaddr*)&arguments->clientAddr, clientLen,arguments->my_datagram.username);

		// receber os dados e salvar no arquivo

		//assembly_server_inst(&arguments->my_datagram.instruction, ACK);
		//sendto(arguments->s, &arguments->my_datagram, sizeof(struct datagram), 0, (struct sockaddr *)&arguments->clientAddr, clientLen);
	}

	if (instruction_id == DOWNLOAD){
		//printf("Servidor entrou no if do DOWNLOAD!\n");

		assembly_server_inst(&arguments->my_datagram.instruction, START_DOWNLOAD);
		assembly_server_inst(&arguments->my_datagram.instruction, ACK);
		sendto(arguments->s, &arguments->my_datagram, sizeof(struct datagram), 0, (struct sockaddr *)&arguments->clientAddr, clientLen);

		send_file(arguments->s,(struct sockaddr*)&arguments->clientAddr, clientLen,arguments->my_datagram.username);

		//assembly_server_inst(&arguments->my_datagram.instruction, ACK);
		//sendto(arguments->s, &arguments->my_datagram, sizeof(struct datagram), 0, (struct sockaddr *)&arguments->clientAddr, clientLen);
	}

	if (instruction_id == BACKUP1){
		fprintf(stderr, "> ENTROU NO IF DO BACKUP1\n");
		pthread_t backup1_thread;
		if ( pthread_create(&backup1_thread, NULL, backup1, NULL) != 0 ) {
			printf("Erro na criação da thread\n");
		}
	}

	if (instruction_id == BACKUP2){
		pthread_t backup2_thread;
		if ( pthread_create(&backup2_thread, NULL, backup2, NULL) != 0 ) {
			printf("Erro na criação da thread\n");
		}
	}

	// ELSE, OUTRAS INSTRUCOES
	//printf("Acabou a thread: %d\n", instruction_id);
	pthread_exit(NULL);

}

int main(int argc, char *argv[]) {

  	struct  sockaddr_in peer;
	SOCKET  s, s_TCP;
	int port;
	int peerlen, n;
	int type;
	char * host;
	fprintf(stderr, "> debug 1\n");
	//Pega paramentro
	if(argc < 3) {
		printf("Utilizar:\n");
		printf("dropBoxServer <port> <1 - primario   2 - backup><ip host>\n");
		exit(1);
	}
	fprintf(stderr, "> debug 2\n");

	port = atoi(argv[1]);  // Porta
	type = atoi(argv[2]);	 // Type
	if(argc == 4){
		host = malloc(strlen(argv[3]));
		strcpy(host, argv[2]); //ip host
	}
	fprintf(stderr, "> debug 3\n");


// eh primario, espera conexao dos secundarios e manda replica dos adicionados
// quando altera dado envia para as replicas um request
	if (type == 1){
		#define Replica1Port 50000
		#define Replica2Port 50001

		// prepara servidor e carrega informacoes (persistencia)
		if ( init_server() == ERROR){
			printf("Erro ao preparar o servidor informacoes do servidor\n");
		}

		s = setup_server(port);
		s_TCP = setup_server_TCP(port);

	} else if(type == 2) {
		int port_tcp = 50000;
	// eh secundario, pede copia do primario
	//fica recebendo request a cada mudanca com a lista de servidores e clientes

	}
		else{
			int port_tcp = 50001;
		}

    // recebe datagrama
    struct datagram received_datagram;
    int rc;
    struct arg_struct *args = NULL;

	SOCKET clientSocket;
   	struct  sockaddr_in clientAddr;
    unsigned int clientLen;
    clientLen = sizeof(clientAddr);

		if (type == 2){
			struct  sockaddr_in peer2;
			SOCKET s2;
			int peerlen2;

			struct datagram my_datagram;

			my_datagram.instruction = 0x40000000;

			if((s2 = socket(AF_INET, SOCK_DGRAM,0)) < 0) {
				printf("Falha na criacao do socket\n");
				return ERROR;
		 	}
			peer2.sin_family = AF_INET;
			peer2.sin_port = htons(port);
			peer2.sin_addr.s_addr = inet_addr(host);
			peerlen2 = sizeof(peer2);

			int rc2 = sendto(s2, &my_datagram, sizeof(struct datagram), 0, (struct sockaddr*) &peer2, peerlen2);

			fprintf(stderr, "> debug 5\n");

			sleep(100);

		}



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
		if (type == 2){
			//espera um pacote com os arquivos do primario
			//se for type 1, a cada operacao, manda o pacote para os type 2 também
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

int setup_server_TCP(int port) {

	struct  sockaddr_in peer_TCP;

	SOCKET s_TCP;

	int peerlen_TCP, n;

	if ((s_TCP = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Falha na criacao do socket tcp\n");
	    exit(1);
 	}

	// Seta pro socket tcp
	memset((void *) &peer_TCP,0,sizeof(struct sockaddr_in));
	peer_TCP.sin_family = AF_INET;
	peer_TCP.sin_addr.s_addr = htonl(INADDR_ANY); // Recebe de qualquer IP
	peer_TCP.sin_port = htons(port); // Recebe na porta especificada na linha de comando
	peerlen_TCP = sizeof(peer_TCP);

	// Associa socket tcp com estrutura peer
	if(bind(s_TCP,(struct sockaddr *) &peer_TCP, peerlen_TCP)) {
			printf("Erro no bind tcp\n");
			exit(1);
	}

	if((listen(s_TCP, 8)) != 0){
		printf("Erro no listen\n");
	}

    printf("Socket TCP inicializado. Aguardando mensagens...\n\n");
    return s_TCP;
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

	//printf("DEBUG: Entrou na receive_files\n");

	rc = recvfrom(s, &fileinfo, sizeof(struct file_info), 0, (struct sockaddr*) peer, (socklen_t *) &peerlen);
	//printf("DEBUG FILE_INFO\nName: %s\nExt: %s\nLast Modified: %s\nSize: %d\n",fileinfo.name,fileinfo.extension,fileinfo.last_modified,fileinfo.size);
	strcpy(buffer_ack, "ACK_FILEINFO");
	//fprintf(stderr,"buffer_ack: %s\n",buffer_ack);
	rc = sendto(s, buffer_ack, 15, 0, (struct sockaddr*) peer, peerlen);

	//fprintf(stderr,"DEBUG: ACK enviado receive_files\n");

	strcat(dir,fileinfo.name);
	//strcat(dir,".");
	strcat(dir,fileinfo.extension);

	//fprintf(stderr,"DEBUG: %s\n", dir);

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

	//printf("DEBUG: Entrou na send_files\n");

	rc = recvfrom(s, &fileinfo, sizeof(struct file_info), 0, (struct sockaddr*) peer, (socklen_t *) &peerlen);
	//printf("DEBUG FILE_INFO\nName: %s\nExt: %s\nLast Modified: %s\nSize: %d\n",fileinfo.name,fileinfo.extension,fileinfo.last_modified,fileinfo.size);

	strcat(dir,fileinfo.name);
	strcat(dir,fileinfo.extension);
	file = fopen(dir, "r");
	if (file == NULL){
			return ERROR;
	}
	//fprintf(stderr,"DEBUG: abriu arquivo send_file\n");

	struct stat st;
	stat(dir, &st);
	int length = st.st_size;
	//fileinfo.size = length;
	fileinfo.size = length;

	//printf("Size: %d\n", length);

	rc = sendto(s, &fileinfo, sizeof(struct file_info), 0, (struct sockaddr*) peer, peerlen);

	//fprintf(stderr,"DEBUG: ACK enviado send_files\n");


	struct datagram pkg = {1,1};
	strcpy(pkg.username, userid);

	while(fread(pkg.buffer,sizeof(char),BUFFER_SIZE,file)) {
			//Envia o 'pkg.buffer'
			//Bloqueia até receber o ack
			//Quando receber o ack, continua no 'while'
	}

	//fprintf(stderr,"BUFFER ARQUIVO: %s\n",pkg.buffer);

	do {
	// envia o file_info
		//fprintf(stderr, "Entrou no while\n");
		rc = sendto(s, &pkg, sizeof(struct datagram), 0, (struct sockaddr*) peer, peerlen);
	// recebe datagrama com ACK
		//rc = recvfrom(s, &pkg, sizeof(struct datagram), 0, (struct sockaddr*) peer, (socklen_t *) &peerlen);
		//printf("PKG.ID: %d\n\n", pkg.id);


	}while (rc < 0); //while (rc < 0 || pkg.id != 2 ); // recebe algo e recebe o ACK do servidor


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
		return EXIT;
	}

	if ( (instruction & 0xf8000000) == 0x40000000 ) {
		return BACKUP1;
	}

	if ( (instruction & 0xf8000000) == 0x48000000 ) {
		return BACKUP2;
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
