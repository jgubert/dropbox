
//#include "dropboxServer.h"

//void sync_server(){}
//void receive_file(char *file){}
//void send_file2(char *file){}

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
	#include <winsock2.h>
#else
	#define SOCKET int
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
#endif

int main(int argc, char *argv) {

    struct  sockaddr_in peer;
	SOCKET  s;
	int port, peerlen, rc;
    char buffer[100];

#ifdef _WIN32
	 WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
		printf("Erro no startup do socket\n");
		exit(1);
	}
#endif    

    // Cria o socket na familia AF_INET (Internet) e do tipo UDP (SOCK_DGRAM)
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("Falha na criacao do socket\n");
	    exit(1);
 	}

    port = 4055; // mudar isso

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
    
    // Recebe pacotes do cliente e responde com string "ACK"
	while (1)
	{ 
	    // Quando recebe um pacote, automaticamente atualiza o IP da estrutura peer
#ifdef _WIN32
		rc = recvfrom(s, buffer, sizeof(buffer), 0, (struct sockaddr *)&peer, &peerlen);
#else
		rc = recvfrom(s,buffer,sizeof(buffer),0,(struct sockaddr *) &peer,(socklen_t *)&peerlen);
#endif
		printf("Recebido %s\n", &buffer);

		strcpy(buffer,"ACK");
		sendto(s,buffer,sizeof(buffer),0,(struct sockaddr *)&peer, peerlen);
	    printf("Enviado ACK\n\n");
	}

}

