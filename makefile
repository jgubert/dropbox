default: client server

client: include/dropboxClient.h include/dropboxClient.h src/dropboxClient.c
	gcc -o bin/dropboxClient src/dropboxClient.c

server: include/dropboxClient.h include/dropboxServer.h src/dropboxServer.c
	gcc -o bin/dropboxServer src/dropboxServer.c

debug:
	gcc -DDEBUG -o bin/dropboxClient src/dropboxClient.c

clean:
	rm bin/*