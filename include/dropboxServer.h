#define MAXNAMES 8
#define MAXNAME 8
#define MAXFILES 10
#define USER_LIMIT 2

int socket_create(char *host, int port);
void sync_server();
//void receive_file(int s, char* user, struct sockaddr_in peer, int peerlen);
//void send_file2(int s, char* user, struct sockaddr_in peer, int peerlen); //ta com o mesmo nome de outra funcao
void create_path(char *user);
int client_count(char *user);



