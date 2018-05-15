#define MAXNAMES 8
#define MAXNAME 8
#define MAXFILES 10

struct file_info{
  char name[MAXNAME];
  char extension[MAXNAME];
  char last_modified[MAXNAME];
  int size;
};

struct client{
  int devices[2];
  char userid[MAXNAMES];
  struct file_info info[MAXFILES];
  int logged_int;
};

int socket_create(char *host, int port);
void sync_server();
void receive_file(char *file, int s, char* user);
void send_file2(char *file, int s, char* user); //ta com o mesmo nome de outra funcao
void create_path(char *user);
int client_count(char *user);
