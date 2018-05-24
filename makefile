default: util client server

util: include/dropboxUtil.h src/dropboxUtil.c
	gcc -c src/dropboxUtil.c
	mv dropboxUtil.o bin

client: include/dropboxClient.h include/dropboxClient.h src/dropboxClient.c
	gcc -o bin/dropboxClient src/dropboxClient.c bin/dropboxUtil.o

server: include/dropboxClient.h include/dropboxServer.h src/dropboxServer.c 
	gcc -o bin/dropboxServer src/dropboxServer.c -lpthread

debug:
	gcc -DDEBUG -o bin/dropboxClient src/dropboxClient.c

clean:
	rm bin/*
