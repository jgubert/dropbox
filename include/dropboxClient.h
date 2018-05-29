#include <stdio.h>
#include <sys/socket.h>

int login_server(char *host, int port);
void sync_client();
//alterei o send_file pra receber o socket
//void receive_file(int s, char* user, struct sockaddr_in peer, int peerlen);
int send_file(char *filename);
int get_file(char *filename);
void delete_file(char *file);
void close_session();
int get_sync_dir();
int assembly_client_inst(int *instruction, int instruction_id);
int login_server(char *host, int port);
int handle_server_connectivity_status(int instruction_id);
int desassembly_server_inst(int word);
int desassembly_server_inst_status(int word, int inst);
int interface();
int create_sync_dir();
