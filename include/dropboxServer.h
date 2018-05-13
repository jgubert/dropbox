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
void receive_file(char *file);
void send_file2(char *file); //ta com o mesmo nome de outra funcao
