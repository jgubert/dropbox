default: client

client:
	gcc -o bin/dropboxClient src/dropboxClient.c

debug:
	gcc -DDEBUG -o bin/dropboxClient src/dropboxClient.c

clean:
	rm bin/*