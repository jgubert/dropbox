#define MAXNAMES 8
#define MAXNAME 8
#define MAXFILES 10

#include <sys/socket.h>

int socket_create(char *host, int port);
void sync_server();
//void receive_file(int s, char* user, struct sockaddr_in peer, int peerlen);
//void send_file2(int s, char* user, struct sockaddr_in peer, int peerlen); //ta com o mesmo nome de outra funcao
void create_path(char *user);
int client_count(char *user);
void receive_file(int s, struct sockaddr* peer, int peerlen);
//void print_package(struct package pacote);
int create_database_structure();
//void receive_file(int s, create_database_structuret sockaddr* peer, int peerlen);
void send_file2(int s, char* user, struct sockaddr* peer, int peerlen);
struct package create_package(int s, struct sockaddr * peer, int peerlen);
int create_database_structure();
int desassembly_client_inst(int instruction);
int assembly_server_inst(int *instruction, int instruction_id);
int setup_server(int port);
void* servidor(void* args);
int is_first_connection(char username[]);
int has_too_many_devices(char username[]);
int init_server();
struct client get_client(char username[]);
int get_client_index(char username[]);
int is_first_connection(char username[]);
int log_device(char username[]);
int log_off_device(char username[]);
