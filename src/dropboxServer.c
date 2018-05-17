
#include "../include/dropboxServer.h"
#include "../include/dropboxUtil.h"
//void sync_server(){}
//void receive_file(char *file){}
//void send_file2(char *file){}

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SUCCESS 1
#define ERROR -1
#define SOCKET int
#define BUFFER_SIZE 12500

#define GET_SYNC_DIR 4

struct client clientes[10];
int semaforo = 0;
char buffer[BUFFER_SIZE];

void receive_file(int s, struct sockaddr* peer, int peerlen);
void send_file2(int s, char* user, struct sockaddr* peer, int peerlen);
struct package create_package(int s, struct sockaddr * peer, int peerlen);

void print_package(struct package pacote);
int create_database_structure();


int main(int argc, char *argv[]) {

  struct  sockaddr_in peer;
	SOCKET  s;
	int port, peerlen, n;

	//Pega paramentro
	if(argc < 2) {
		printf("Utilizar:\n");
		printf("dropBoxServer <port>");
		exit(1);
	}

	port = atoi(argv[1]);  // Porta

    // Cria o socket na familia AF_INET (Internet) e do tipo UDP (SOCK_DGRAM)
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

    create_database_structure();
	printf("Criou database!\n\n");

	// ------------ TESTANDO GET_SYNC_DIR ----------
	
	while(1) {
	ssize_t teste;

	struct client cliente;

	printf("Esperando pacote!\n");

	teste = recvfrom(s, &cliente, 12500, 0, (struct sockaddr *) &peer,(socklen_t *)&peerlen);
		

	strcpy(buffer,"ACK");

	if(teste != -1)
		printf("Pacote recebido! %d\n", teste);
	else{	
		printf("Pacote não recebido!\n");
	}
	//sleep(1);
    n = sendto(s,buffer,sizeof(buffer),0,(struct sockaddr *) &peer, peerlen);
	if(n < 0){
		printf("Erro no envio do ACK!\n");
	}
	
	if(cliente.command_id == GET_SYNC_DIR){

		create_path(cliente.userid);
	
	}

}
	
	// ------------ FIM TESTE ----------------------


    receive_file(s, (struct sockaddr *) &peer, peerlen);

	return 0;

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

struct package create_package(int s, struct sockaddr * peer, int peerlen){
  char buffer[1250];
  int rc;
  struct package pacote;
  // Recebe pacotes do cliente e responde com string "ACK"
  while (1)
  {
    // Quando recebe um pacote, automaticamente atualiza o IP da estrutura peer

    //rc = recvfrom(s,buffer,sizeof(buffer),0,(struct sockaddr *) &peer,(socklen_t *)&peerlen);
    rc = recvfrom(s, &pacote, sizeof(struct package), 0, (struct sockaddr *) &peer,(socklen_t *)&peerlen);
    //printf("Recebido %s\n", &buffer);

    //print_package(pacote);

    printf("Usuario %s enviou um pacote\n", pacote.username);
    printf("Informacoes da instrucao\n");
    printf("%d\n", pacote.command.command_id);
    printf("%s\n", pacote.command.path);
    printf("%s\n", pacote.command.filename);
    printf("Quantidade de bytes recebidos: %ld\n", sizeof(pacote.buffer));


    strcpy(buffer,"ACK");
    sendto(s,buffer,sizeof(buffer),0,(struct sockaddr *)&peer, peerlen);
    printf("Enviado ACK\n\n");
  }
  return pacote;
}

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

	struct stat st = {0};
	char dir[50] = "database/sync_dir_";
	strcat(dir, user);

	if(mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0)
		printf("Pasta %s criada.\n", dir);
	else printf("Pasta %s já existe.\n",dir);

	return;
}

void print_package(struct package pacote) {
	printf("Usuario %s enviou um pacote\n", pacote.username);
	printf("Informacoes da instrucao\n");
	printf("%d\n", pacote.command.command_id);
	printf("%s\n", pacote.command.path);
	printf("%s\n", pacote.command.filename);
	printf("Quantidade de bytes recebidos: %ld\n", sizeof(pacote.buffer));
}
