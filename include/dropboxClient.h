#include <stdio.h>
#include <sys/socket.h>

int login_server(char *host, int port);
void sync_client();
//alterei o send_file pra receber o socket
//void receive_file(int s, char* user, struct sockaddr_in peer, int peerlen);
//void send_file(char *file, int s, struct sockaddr_in peer, int peerlen);
void get_file(char *file);
void delete_file(char *file);
void close_session();
int get_sync_dir();
int assembly_client_inst(int *instruction, int instruction_id);
int login_server(char *host, int port);
int handle_server_connectivity_status(int instruction_id);
int desassembly_server_inst(int word);
int interface();
