


default: program

program:
	gcc -o bin/dropboxClient src/dropboxClient.c

debug:
	gcc -DDEBUG -o bin/dropboxClient src/dropboxClient.c