#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

//#include "threadsfun.h";

#define PORT 4007

void *sendSocket(void *vargp);
void *receiveSocket(void *vargp);

// esse trecho era na main, declarei global pra usar o socket
//nas funcoes de threads


    int sockfd, n;
	unsigned int length;
	struct sockaddr_in serv_addr, from;
	struct hostent *server;

//até aqui.

int main(int argc, char *argv[])
{

	
	char buffer[256];
	if (argc < 2) {
		fprintf(stderr, "usage %s hostname\n", argv[0]);
		exit(0);

	}
	
	server = gethostbyname(argv[1]);
	if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }	
	
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		printf("ERROR opening socket");
	
	serv_addr.sin_family = AF_INET;     
	serv_addr.sin_port = htons(PORT);    
	serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(serv_addr.sin_zero), 8);  

	printf("Enter the message: ");
	bzero(buffer, 256);
	fgets(buffer, 256, stdin);

	

	n = sendto(sockfd, buffer, strlen(buffer), 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
	if (n < 0) 
		printf("ERROR sendto");
	
	length = sizeof(struct sockaddr_in);
	n = recvfrom(sockfd, buffer, 256, 0, (struct sockaddr *) &from, &length);
	if (n < 0)
		printf("ERROR recvfrom");


	printf("Got an ack: %s\n", buffer);

	
	fgets(buffer, 256, stdin);

	pthread_t thread_id1, thread_id2;

	printf("Criando Thread Send.\n");

	pthread_create(&thread_id1, NULL, sendSocket, buffer);
	pthread_join(thread_id1, NULL);


	printf("Criando Thread Receive.\n");

	pthread_create(&thread_id2, NULL, receiveSocket, buffer);
	pthread_join(thread_id2, NULL);
	
	close(sockfd);
	return 0;
}

// ---- FUNCOES DE ENVIO E RECEBIMENTO EM THREADS.


void *sendSocket(void *vargp){
	int n;

	fprintf(stderr, "- DEBUG - Entrou na função sendSocket!\n");
	fprintf(stderr, "- DEBUG - Buffer: %s\n", vargp);

	n = sendto(sockfd, vargp, strlen(vargp), 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
	if (n < 0) 
		printf("ERROR sendto");

	fprintf(stderr, "- DEBUG - Enviou na função sendSocket!\n");

	pthread_exit(NULL);

}

void *receiveSocket(void *vargp){

	int n;

	fprintf(stderr, "- DEBUG - Entrou na função receiveSocket!\n");
	
	n = recvfrom(sockfd, vargp, strlen(vargp), 0, (struct sockaddr *) &from, &length);
	if (n < 0)
		printf("ERROR recvfrom");

	fprintf(stderr, "- DEBUG - Recebeu na função receiveSocket!\n");
		fprintf(stderr, "- DEBUG - Buffer: %s\n", vargp);

	pthread_exit(NULL);


}


