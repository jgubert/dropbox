#include <sys/socket.h>

int socket_create(char *host, int port);
void sync_server();
void create_path(char *user);
int client_count(char *user);
void receive_file(char *file, int s, struct sockaddr* peer, int peerlen, char* userid);
int create_database_structure();
int send_file(int s, struct sockaddr* peer, int peerlen, char* userid);
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
int save_clients();
int load_clients();
