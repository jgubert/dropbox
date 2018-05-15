#include <stdio.h>
#include <sys/socket.h>

int login_server(char *host, int port);
void sync_client();
//alterei o send_file pra receber o socket
(int s, char* user, struct sockaddr_in peer, int peerlen)
void send_file(char *file, int s, struct sockaddr_in peer, int peerlen);
void get_file(char *file);
void delete_file(char *file);
void close_session();
