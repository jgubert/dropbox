
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


int login_server(char *host, int port);
void sync_client();
void send_file(char *file);
void get_file(char *file);
void delete_file(char *file);
void close_session();
