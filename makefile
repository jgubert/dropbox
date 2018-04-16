default: client

client: include/dropboxClient.h src/dropboxClient.c
	gcc -o bin/dropboxClient src/dropboxClient.c

client: include/dropboxServer.h src/dropboxServer.c
	gcc -o bin/dropboxServer src/dropboxServer.c

debug:
	gcc -DDEBUG -o bin/dropboxClient src/dropboxClient.c

clean:
	rm bin/*