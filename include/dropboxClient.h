#include <stdio.h>

#define MAXNAMES 8
#define MAXNAME 8
#define MAXFILES 10


int login_server(char *host, int port);
void sync_client();
void send_file(char *file);
void get_file(char *file);
void delete_file(char *file);
void close_session();
