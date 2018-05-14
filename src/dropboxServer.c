
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


#define SOCKET int
struct client clientes[10];
int semaforo = 0;

void print_package(struct package pacote);


int main(int argc, char *argv[]) {

  struct  sockaddr_in peer;
	SOCKET  s;
	int port, peerlen, rc;
  char buffer[100];

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


/*void receive_file(char *file){
	FILE* file_complete;
	printf("Socket inicializado. Aguardando mensagens...\n\n");
	ssize_t bytes_receive = 0; //quantidade de bytes recebidos ate entao
	ssize_t bytes_send = 0; //foi enviado ate entao

	//printf("Usuario %s enviou um pacote\n", client.userid);
	char dir[100] = "sync_dir_";
	//strcat(dir, client.userid);
	strcat(dir, "/");

	//receber o nome do arquivo que vai receber do cliente
	//file_info.name = nome do arquivo


	//strcat(dir, buffer);
	bytes_receive = 0;
	file_complete = fopen(dir, "w");
	//zera buffer

	//while ((bytes_receive = recv(s, buffer, BUFFER_SIZE, 0)) > 0){
	//	fwrite(buffer, 1, bytes_receive, file_complete);
	//	//zera buffer
	//	if(bytes_receive < BUFFER_SIZE){
	//		fclose(file_complete);
	//		printf("SUCESSO");
	//	}
	//}


}*/

void create_path(char *user){

	struct stat st = {0};
	char dir[50] = "sync_dir_";
	strcat(dir, user);

	if(stat(dir, &st) != 0){
		mkdir(dir, 07777);
		printf("pasta criada para user %s", user);
	}

	else{
		printf("ja existe pasta para user %s", user);
	}
}

void print_package(struct package pacote) {
	printf("Usuario %s enviou um pacote\n", pacote.username);
	printf("Informacoes da instrucao\n");
	printf("%d\n", pacote.command.command_id);
	printf("%s\n", pacote.command.path);
	printf("%s\n", pacote.command.filename);
	printf("Quantidade de bytes recebidos: %ld\n", sizeof(pacote.buffer));
}
