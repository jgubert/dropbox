#include "../include/dropboxServer.h"
#include "../include/dropboxUtil.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define GET_SYNC_DIR 4
#define Replica1Port 50000
#define Replica2Port 50001

#define TypeServer		1
#define TypeBackup		2

#define BackupServerNewFileMessage		0
#define BackupServerClientFileUpdate	1

#define PrimaryServerNewFileMessage		0
#define PrimaryServerServerList			1

#define MaxUDPDatagramSize				1024

#define MaximumBackupServers			100

#define FileFolder	"Database"

// variaveis globais de controle do servidor
struct client clients[10];
int semaforo = 0;
char buffer[BUFFER_SIZE];

// Os dados de entrada do programa (input port and ip host)
int s_InputPort;
char s_InputHost[100];
int s_ClientConnectPort;

// Todas as conexoes de servidores de backup (sockets)
int s_BackupSockets[MaximumBackupServers];
int s_BackupSocketsInUse = 0;

// Essa é o socket de conexao com o servidor principal, utilizado pelos backups
int s_PrimaryServerSocket = -1;

//Array de servidores
struct server servers[100]; 


// --- FUNÇÕES AUXILIARES ---

int create_database_structure();
void create_path(char *user);
// int setup_backup_tpc_server(int port);
int init_primary_server_client_list();
int create_folder(char* _name);
int setup_primary_server(int port);
int config_backup_server(SOCKET serverSocket);
void* listen_client_messages(void* args);
void* listen_server_messages(void* args);
void* listen_backup_tcp_requests(void* args);
int setup_client_listen_server(int port);

void* backup1(void* args){

}

void* backup2(void* args){

}

void* listenFE(void* args){

	struct arg_portas *arguments = (struct arg_portas *)args;

	int sockfd, newsockfd, n;
	socklen_t clilen;
	char buffer[256];
	struct sockaddr_in addr_serv, addr_cli;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
			printf("ERROR opening socket");


			// seta informacoes IP/Porta locais
			addr_cli.sin_family = AF_INET;
			addr_cli.sin_addr.s_addr = htonl(INADDR_ANY);
			addr_cli.sin_port = htons(arguments->portaCli); //porta cliente

			// associa configuracoes locais com socket
			if (bind(sockfd, (struct sockaddr *) &addr_serv, sizeof(addr_serv)) < 0)
				printf("ERROR on binding");

		  // seta informacoes IP/Porta do servidor remoto
		  addr_serv.sin_family = AF_INET;
		  addr_serv.sin_addr.s_addr = inet_addr(&arguments->IPServ);
		  addr_serv.sin_port = htons(arguments->portaServ);
			bzero(&(addr_serv.sin_zero), 8);


		listen(sockfd, 8);

		clilen = sizeof(struct sockaddr_in);
	if ((newsockfd = accept(sockfd, (struct sockaddr *) &addr_cli, &clilen)) == -1)
		printf("ERROR on accept");
	
	bzero(buffer, 256);

	/* read from the socket */
	n = read(newsockfd, buffer, 256);
	if (n < 0)
		printf("ERROR reading from socket");
	printf("Here is the message: %s\n", buffer);

	/* write in the socket */
	n = write(newsockfd,buffer, 256);
	if (n < 0)
		printf("ERROR writing to socket");

	close(newsockfd);
	close(sockfd);


}

void* servidor(void* args) {

	struct arg_struct *arguments = (struct arg_struct *)args;
	int instruction_id;
	int clientLen = sizeof(arguments->clientAddr);

	//printf("DEBUG instrucao recebido em hex: %x\n", arguments->my_datagram.instruction);

	instruction_id = desassembly_client_inst(arguments->my_datagram.instruction);
	//printf("Instruction id: %d\n", instruction_id);

	if (instruction_id == ESTABLISH_CONNECTION) {

		// verifica se usuario existe no sistema
		if ( is_first_connection(arguments->my_datagram.username) ){

			// get available client slot
			int i = 0;
			for(i=0; i<MAXUSERS; i++) {
				if ( strcmp(clients[i].userid, "") == 0 ){
					break;
				}
			}
			// se o ultimo estiver preenchido
			if ( (i == MAXUSERS-1) && (strcmp(clients[MAXUSERS-1].userid, "") != 0) ){
				assembly_server_inst(&arguments->my_datagram.instruction, TOO_MANY_USERS);
			}
			else{
				strcpy(clients[i].userid, arguments->my_datagram.username);
				clients[i].devices[0] = TRUE;
				save_clients();

				// CRIAR DIRETORIO USERNAME AQUI
				create_database_structure();
				create_path(clients[i].userid);

				assembly_server_inst(&arguments->my_datagram.instruction, FIRST_TIME_USER);
			}
		}

		else {
			int device_index = log_device(arguments->my_datagram.username);
			int client_index = get_client_index(arguments->my_datagram.username);

			if ( device_index != ERROR) {
				if (device_index == 0)
					clients[client_index].logged_in = TRUE;
					save_clients();
				assembly_server_inst(&arguments->my_datagram.instruction, CONNECTED);
			}
			else {
				assembly_server_inst(&arguments->my_datagram.instruction, TOO_MANY_DEVICES);
			}
		}

		assembly_server_inst(&arguments->my_datagram.instruction, ACK);
		sendto(arguments->s, &arguments->my_datagram, sizeof(struct datagram), 0, (struct sockaddr *)&arguments->clientAddr, clientLen);

		// sair da thread
	}

	if (instruction_id == EXIT) {

		int client_index = get_client_index(arguments->my_datagram.username);

		if (log_off_device(arguments->my_datagram.username) == 0){ // se deslogar do device cujo index é 0
			clients[client_index].logged_in = FALSE;
		}

		save_clients();

		assembly_server_inst(&arguments->my_datagram.instruction, TERMINATE_CLIENT_EXECUTION);
		assembly_server_inst(&arguments->my_datagram.instruction, ACK);
		sendto(arguments->s, &arguments->my_datagram, sizeof(struct datagram), 0, (struct sockaddr *)&arguments->clientAddr, clientLen);

	}

	if (instruction_id == UPLOAD) {
		//printf("Servidor entrou no if do UPLOAD!\n");

		// FILE_RECEIVED é temporario.. primeiro envia o ACK para começar o envio do arquivo.
		assembly_server_inst(&arguments->my_datagram.instruction, START_SENDING);
		assembly_server_inst(&arguments->my_datagram.instruction, ACK);
		sendto(arguments->s, &arguments->my_datagram, sizeof(struct datagram), 0, (struct sockaddr *)&arguments->clientAddr, clientLen);

		receive_file(arguments->my_datagram.file.name,arguments->s,(struct sockaddr*)&arguments->clientAddr, clientLen,arguments->my_datagram.username);

		// receber os dados e salvar no arquivo

		//assembly_server_inst(&arguments->my_datagram.instruction, ACK);
		//sendto(arguments->s, &arguments->my_datagram, sizeof(struct datagram), 0, (struct sockaddr *)&arguments->clientAddr, clientLen);
	}

	if (instruction_id == DOWNLOAD){
		//printf("Servidor entrou no if do DOWNLOAD!\n");

		assembly_server_inst(&arguments->my_datagram.instruction, START_DOWNLOAD);
		assembly_server_inst(&arguments->my_datagram.instruction, ACK);
		sendto(arguments->s, &arguments->my_datagram, sizeof(struct datagram), 0, (struct sockaddr *)&arguments->clientAddr, clientLen);

		send_file(arguments->s,(struct sockaddr*)&arguments->clientAddr, clientLen,arguments->my_datagram.username);

		//assembly_server_inst(&arguments->my_datagram.instruction, ACK);
		//sendto(arguments->s, &arguments->my_datagram, sizeof(struct datagram), 0, (struct sockaddr *)&arguments->clientAddr, clientLen);
	}

	if (instruction_id == BACKUP1){
		fprintf(stderr, "> ENTROU NO IF DO BACKUP1\n");
		pthread_t backup1_thread;
		if ( pthread_create(&backup1_thread, NULL, backup1, NULL) != 0 ) {
			printf("Erro na criação da thread\n");
		}
	}

	if (instruction_id == BACKUP2){
		pthread_t backup2_thread;
		if ( pthread_create(&backup2_thread, NULL, backup2, NULL) != 0 ) {
			printf("Erro na criação da thread\n");
		}
	}

	// ELSE, OUTRAS INSTRUCOES
	//printf("Acabou a thread: %d\n", instruction_id);
	pthread_exit(NULL);

}

void aux_tcp_read(SOCKET socket, unsigned int size, void* buffer)
{
	int bytesRead = 0;
	int result;
	
	while(bytesRead < size)
	{
		result = read(socket, buffer + bytesRead, size - bytesRead);
		{
			if(result < 1)
			{
				// Error
				// ...
			}
		}

		bytesRead += result;
	}
}

int aux_tcp_write(SOCKET socket, unsigned size, void* data)
{
	int	n = write(socket,data, size);
	if (n < 0)
	{
		printf("ERROR writing to socket");
	}

	return n;
}

// Le o tamanho da mensagem e logo em seguida a mensagem propriamente dita, 
// retornando o tamanho dela e atuaizando/alocando os dados no buffer (parametro)
int tcp_read(SOCKET socket, int* messageType, void** buffer)
{
	int length = 0;
	
	// Read the message size
	aux_tcp_read(socket, sizeof(length), (void*)&length);

	// Read the message type
	aux_tcp_read(socket, sizeof(messageType), (void*)messageType);

	// Allocate the data
	*buffer = malloc(length);

	// Read the message size
	aux_tcp_read(socket, length, *buffer);

	return length;
}

// Envia o tamanho da mensagem e logo em seguida a mensagem propriamente dita
void tcp_write(SOCKET socket, int messageType, unsigned size, void* data)
{
	int length = size;

	// Write the message length
	aux_tcp_write(socket, sizeof(length), (void*)&length);

	// Write the message type
	aux_tcp_write(socket, sizeof(messageType), (void*)&messageType);

	// Write the message itself
	aux_tcp_write(socket, size, data);
}

int udp_read(SOCKET socket, struct sockaddr_in* clientAddr, int* messageType, void** buffer)
{
	// Cria um buffer temporario geral e outro local
	*buffer = malloc(MaxUDPDatagramSize);
	char* localBuffer = (char*)malloc(MaxUDPDatagramSize + sizeof(int));

	// Estrutura de info do cliente que receberemos a mensagem
	unsigned int clientLen = sizeof(&clientAddr);

	// NAO USADO ... Ajusta um tempo de timeout
	// int timeout = 1000;
	// setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
	
	// Recebe uma nova mensagem
	int rc = recvfrom(socket, localBuffer, sizeof(struct datagram), 0, (struct sockaddr *) clientAddr,(socklen_t *)&clientLen);
	if (rc < 0) 
	{
		printf("Erro ao receber datagrama\n");
		return ERROR;
	}

	// Ajusta o tipo de mensagem
	*messageType = *(int*)localBuffer;

	// Copia os dados
	memcpy(*buffer, &localBuffer[sizeof(int)], rc);

	// Deleta o buffer local
	free(localBuffer);

	return rc - sizeof(int);
}

// TODO: Mudar o frontend para usar essa funcao
int udp_write(SOCKET socket, int port, char* host, unsigned size, int messageType, void* datagram)
{
	struct  sockaddr_in peer;
	int peerlen;

	// Cria um buffer temporario que ira ter o tipo de mensagem e a mensagem propriamente dita
	char* buffer = malloc(MaxUDPDatagramSize + sizeof(int));
	
	// Copia o tipo de mensagem
	memcpy(buffer, &messageType, sizeof(int));

	// Copia os dados da mensagem
	memcpy(&buffer[sizeof(int)], datagram, size);

	peer.sin_family = AF_INET;
	peer.sin_port = htons(port);
	peer.sin_addr.s_addr = inet_addr(host);
	peerlen = sizeof(peer);

	int rc = sendto(socket, datagram, size, 0, (struct sockaddr*) &peer, peerlen);

	// Limpa o buffer temporario
	free(buffer);

	return rc;
}

int main(int argc, char *argv[]) {

  	struct  sockaddr_in peer;
	int port;
	int peerlen, n;
	int type;
	char* host;

	// Socket usado pelo servidor pricipal para receber mensagens de clientes
	SOCKET  serverListenSocket;

	// Socket usado na comunicacao do backup com o servidor, contem a comunicacao tcp com o server
	// primario
	SOCKET serverSocket;

	///////////////////
	// INICIALIZACAO //
	///////////////////

	//Pega paramentro
	if(argc < 3) {
		printf("Utilizar:\n");
		printf("dropBoxServer <1 - primario> 	<client connect port> <backup tcp listen port>\n");
		printf("ou:\n");
		printf("dropBoxServer <2 - backup> 		<client connect port> <backup tcp connect port> <primary server ip host>\n");
		exit(1);
	}

	// Get the default values
	type 				= atoi(argv[1]);
	s_ClientConnectPort = atoi(argv[2]);
	s_InputPort 		= atoi(argv[3]);

	if(argc == 5){
		host = malloc(strlen(argv[4]));
		strcpy(host, argv[4]); //ip host
		strcpy(s_InputHost, argv[4]);
	}

	////////////////////////////
	// INICIALIZA COMUNICACAO //
	////////////////////////////

	// eh primario, espera conexao dos secundarios e manda replica dos adicionados
	// quando altera dado envia para as replicas um request
	if (type == TypeServer)
	{
		printf("Somor um servidor principal...");
		
		// Inicializa a lista de clientes para o servidor primario
		if ( init_primary_server_client_list() == ERROR)
		{
			printf("Erro ao preparar o servidor com informacoes dos clientes\n");
		}

		printf("Inicializacao concluida!\n");
	} 
	else if(type == TypeBackup) 
	{
		printf("Somos um servidor de backup...");

		// Precisa de algo aqui?
		// ...

		printf("Inicializacao concluida!\n");
	}
	else
	{
		// Tipo invalido, exit
		printf("Tipo invalido informado\n");
		exit(0);
	}

	// Cria a pasta raiz do servidor de arquivos
	create_folder(FileFolder);

	// Abre a conexao UDP do servidor primaria (a que sera usada pelos clientes)
	serverListenSocket = setup_client_listen_server(s_ClientConnectPort);

	// No caso da gente ser um servidor de backup
	if(type == TypeBackup)
	{
		// Devemos realizar uma conexao com o servidor principal!! //

		// Cria o socket para conexao com o server principal
		int primaryServerSocket = -1;
		if((s_PrimaryServerSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			printf("Erro criando socket para conexao com o servidor principal!\n");
		}

		// Ajusta os dados para conexao com o servidor
		struct sockaddr_in serv_addr;
		memset(&serv_addr, '0', sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(s_InputPort);
		serv_addr.sin_addr.s_addr = inet_addr(s_InputHost);

		printf("Conectando ao servidor principal na porta %d e ip %s\n", s_InputPort, s_InputHost);

		// Tenta se conectar com o servidor principal
		if(connect(s_PrimaryServerSocket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
		{
			printf("Erro ao conectar com o servidor principal!\n");
		}

		// Configura o servidor de backup (nos) recebendo a lista de clientes
		config_backup_server(s_PrimaryServerSocket);
	}
	else
	{
		// Devemos criar uma thread que ira esperar por requisicoes de conexao tcp de servidores
		// de backup
		pthread_t thread;
		if (pthread_create(&thread, NULL, listen_backup_tcp_requests, NULL) != 0 ) 
		{
			printf("Erro na criação da thread\n");
		}
	}
	
	// Chama a funcao que espera por mensagens de clientes (bloqueando a continuacao do main aqui)
	listen_client_messages(&serverListenSocket);
}

// Update the client.dat 
void UpdateClientData(int dataSize, char* data)
{
	char dir[100] = "database/sync_dir_";
	strcat(dir,"/");
	struct file_info fileinfo;

	strcpy(fileinfo.name, "client");
	strcpy(fileinfo.name, ".dat");

	strcat(dir,fileinfo.name);
	strcat(dir,fileinfo.extension);

	FILE* file = fopen(dir, "r");
	if (file == NULL)
	{
		printf("Erro realizando o update do client.data\n");
	}

	fwrite(data, dataSize, 1, file);

	fclose(file);
}

void* listen_messages_from_primary_server(void* args)
{
	// Transforma a variavel socket (vinda dos args)
	SOCKET socket = *(SOCKET*)args;

	// Espera por uma mensagem do servidor principal (TCP)
	while(1)
	{
		int messageType;
		char* buffer;

		// Espera ate receber uma mensagem do servidor principal
		int messageSize = tcp_read(socket, &messageType, (void**)&buffer);
			
		// Verifica se é um arquivo novo ou uma atualizacao do client.dat
		if(messageType == BackupServerNewFileMessage)
		{
			printf("Novo arquivo recebido, do servidor principal\n");

			// Coloca esse novo arquivo na pasta backup
			// TODO: ...
		}
		// É uma atualizacao do client.dat (BackupServerClientFileUpdate)
		else
		{
			printf("Atualizacao do client.dat recebida! Atualizando arquivo...\n");

			// Atualiza o client.dat
			UpdateClientData(messageSize, buffer);
		}

		// Deleta a array temporaria
		free(buffer);
	}
}

void* listen_backup_tcp_requests(void* args)
{
	int sockfd;
	struct sockaddr_in serverAddress;

	printf("Inicializando a escuta para novas conexoes de servidores de backup... ");

	// Cria o socket que usaremos para receber as conexoes
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("ERROR opening socket -> tcp_requests\n");
		exit(0);
	}

	// seta informacoes IP/Porta do servidor remoto
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY); // inet_addr(s_InputHost)
	serverAddress.sin_port = htons(s_InputPort);
	bzero(&(serverAddress.sin_zero), 8);	

	// associa configuracoes locais com socket
	if (bind(sockfd, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0)
	{
		printf("ERROR on binding -> tcp_requests\n");
		exit(0);
	}		
			
	// Listen to this socket
	listen(sockfd, 8);

	printf("Inicializacao concluida, listen ativo!\n");

	// Fica aceitando conexoes
	while(1)
	{
		struct sockaddr_in backupAddress;
		socklen_t size = sizeof(backupAddress);

		printf("Esperando por nova conexao de um servidor de backup...\n");

		// Accept a new connection
		int newSocket = accept(sockfd, (struct sockaddr*) &backupAddress, &size);

		// Print
		printf("Novo servidor de backup connectado na porta: %d\n", newSocket);

		// Add this new backup socket to our array
		s_BackupSockets[s_BackupSocketsInUse] = newSocket;

		// Increment the socket count
		s_BackupSocketsInUse++;

		// Envia todos os dados necessarios para esse novo servidor de backup
		// TODO: ... do servidor principal para os server de backup (client.dat)
	}
}

void* listen_client_messages(void* args)
{
	// Transforma a variavel socket (vinda dos args)
	SOCKET clientListenSocket = *(SOCKET*)args;

	// ...
	while(1)
	{
		// A estrutura do cliente que sera atualizada quando recebermos uma nova mensagem
		struct sockaddr_in clientAddr;
		int messageType;
		void* data;
		
		// Recebe uma nova mensagem do cliente
		int messageSize = udp_read(clientListenSocket, &clientAddr, &messageType, &data);
		
		// Verifica se a mensagem é um cliente adicionando o arquivo novo
		if(messageType == PrimaryServerNewFileMessage)
		{
			printf("Novo arquivo recevido do cliente, atualizando arquivo...\n");

			// Devemos salvar esse arquivo que recebeu do cliente
			// TODO: ...

			printf("Enviando arquivo recebido para %d servidores de backup.\n", s_BackupSocketsInUse);

			// Devemos enviar para os servidores de backup esse arquivo
			for(int i=0; i<s_BackupSocketsInUse; i++)
			{
				// Envia a mensagem de arquivo novo para esse servidor de backup
				tcp_write(s_BackupSockets[i], BackupServerNewFileMessage, messageSize, data);
			}

			printf("Servidores de backup atualizados!\n");
		}
		// É um envio da lista de servidores pelo lado do cliente (devemos assumir como servidor
		// principal) (PrimaryServerServerList)
		else
		{
			printf("Lista de servidores recebida, devemos assumir como servidor principal!\n");

			// Se recebemos esse arquivo, nos devemos ser o novo "servidor principal", logo
			// precisamos abrir uma conexao com os servidores de backup recebidos nessa lista
			// Antes disso devemos criar uma thread que ira esperar por requisicoes de conexao 
			// tcp de servidores de backup
			pthread_t thread;
			if (pthread_create(&thread, NULL, listen_backup_tcp_requests, NULL) != 0 ) 
			{
				printf("Erro na criação da thread -> listen_backup_tcp_requests\n");
			}

			// Agora precisamos de alguma forma avisar os outros servidores de backup
			// que eles devem cancelar a conexao antiga com o antigo servidor principal 
			// (afinal ele näo está mais acessível) e que devem se conectar com esse aqui 
			// (acabamos de criar uma thread logo acima para receber essas conexoes), 
			// podemos usar a lista de servidores que o cliente tem (e que foi recebida
			// aqui) de forma a saber quais sao os enderecos e portas desses servidores 
			// de backup e avisar eles que devem se conectar com esse aqui.

			// TODO: Fazer isso que foi dito acima, existem varias formas mas a mais 
			// facil que eu penso agora seria enviar uma mensagem pela porta UDP (que 
			// cada servidor de backup tem aberta) dizendo que ele deve se conectar com
			// tal endereco e porta (mandar na mensagem isso), essa conexao UDP deveria
			// ser utilizada apenas para clientes mas nesse caso seria uma gambiarra 
			// aceitável... Enfim, precisamos fazer de alguma forma que os outros 
			// servidores de backup se conectem com o novo primario

			printf("Conexoes com servidores de backup abertas!!\n");

			// Devemos fechar a thread que espera pelas mensagens do servidor principal (afinal 
			// nos somos o servidor principal agora) e enviar uma mensagem para 
			// TODO: Podemos salvar o pthread variable globalmente e aqui fechá-lo
		}

		// Limpa os dados temporarios
		free(data);
	}
}

int setup_client_listen_server(int port) {

	struct  sockaddr_in peer;

	SOCKET s;

	int n;

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("Falha na criacao do socket\n");
	    exit(1);
 	}

	 printf("Abrindo socket para receber mensagens de cliente na porta: %d\n", port);

    // Define domínio, IP e porta a receber dados
	memset((void *) &peer,0,sizeof(struct sockaddr_in));
	peer.sin_family = AF_INET;
	peer.sin_addr.s_addr = htonl(INADDR_ANY); // Recebe de qualquer IP
	peer.sin_port = htons(port + 1000); // Recebe na porta especificada na linha de comando

	// Associa socket com estrutura peer
	if(bind(s,(struct sockaddr *) &peer, sizeof(peer))) {
	    printf("Erro no bind\n");
	    exit(1);
	}

    printf("Socket inicializado. Aguardando mensagens de clientes...\n");
    return s;
}

int config_backup_server(SOCKET serverSocket)
{
	// Recebe o arquivo clients.dat do servidor principal
	// ...

	// TODO: Precisamos receber esse arquivo do servidor principal e salva-lo...
}

/*
int setup_backup_tpc_server(int port) {

	struct  sockaddr_in peer_TCP;

	SOCKET s_TCP;

	int peerlen_TCP, n;

	if ((s_TCP = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Falha na criacao do socket tcp\n");
	    exit(1);
 	}

	// Seta pro socket tcp
	memset((void *) &peer_TCP,0,sizeof(struct sockaddr_in));
	peer_TCP.sin_family = AF_INET;
	peer_TCP.sin_addr.s_addr = htonl(INADDR_ANY); // Recebe de qualquer IP
	peer_TCP.sin_port = htons(port); // Recebe na porta especificada na linha de comando
	peerlen_TCP = sizeof(peer_TCP);

	// Associa socket tcp com estrutura peer
	if(bind(s_TCP,(struct sockaddr *) &peer_TCP, peerlen_TCP)) {
			printf("Erro no bind tcp\n");
			exit(1);
	}

	if((listen(s_TCP, 8)) != 0){
		printf("Erro no listen\n");
	}

    printf("Socket TCP inicializado. Aguardando mensagens...\n\n");
    return s_TCP;
}
*/

int init_primary_server_client_list() {

	// carrega a lista de clientes
	if(access("clients.dat", F_OK ) != -1)
		load_clients();
	else{
		for(int i=0; i<MAXUSERS; i++){
			clients[i].devices[0] = 0;
			clients[i].devices[1] = 0;
			strcpy(clients[i].userid, "");
			clients[i].logged_in = 0;
			save_clients();
		}
	}
}

int create_folder(char* _name)
{
	if(mkdir(_name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0)
		//printf("Pasta %s criada.\n", dir_name);

	{}
	else{
		//printf("Pasta %s já existe.\n", dir_name);
		return ERROR;
	}
	return SUCCESS;
}

int create_database_structure() {

	char dir_name[20] = "database";

	if(mkdir(dir_name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0)
		//printf("Pasta %s criada.\n", dir_name);

	{}
	else{
		//printf("Pasta %s já existe.\n", dir_name);
		return ERROR;
	}
	return SUCCESS;

}

void receive_file(char *file, int s, struct sockaddr* peer, int peerlen, char *userid){
	int rc;
	struct file_info fileinfo;
	char buffer_ack[15];
	bzero(buffer_ack,15);
	struct datagram pkg;

	char dir[100] = "database/sync_dir_";

	strcat(dir, userid);
	strcat(dir,"/");

	//printf("DEBUG: Entrou na receive_files\n");

	rc = recvfrom(s, &fileinfo, sizeof(struct file_info), 0, (struct sockaddr*) peer, (socklen_t *) &peerlen);
	//printf("DEBUG FILE_INFO\nName: %s\nExt: %s\nLast Modified: %s\nSize: %d\n",fileinfo.name,fileinfo.extension,fileinfo.last_modified,fileinfo.size);
	strcpy(buffer_ack, "ACK_FILEINFO");
	//fprintf(stderr,"buffer_ack: %s\n",buffer_ack);
	rc = sendto(s, buffer_ack, 15, 0, (struct sockaddr*) peer, peerlen);

	//fprintf(stderr,"DEBUG: ACK enviado receive_files\n");

	strcat(dir,fileinfo.name);
	//strcat(dir,".");
	strcat(dir,fileinfo.extension);

	//fprintf(stderr,"DEBUG: %s\n", dir);

	FILE * write_file;
    write_file = fopen (dir, "w");
    if (write_file == NULL) {
        printf("ERRO AO ABRIR O ARQUIVO PARA ESCRITA!\n");
    }

	rc = recvfrom(s, &pkg, sizeof(struct datagram), 0, (struct sockaddr*) peer, (socklen_t *) &peerlen);
	pkg.id = 2;
	rc = sendto(s, &pkg, sizeof(struct datagram), 0, (struct sockaddr*) peer, peerlen);

	fwrite(pkg.buffer, fileinfo.size, 1, write_file);
	fclose(write_file);
}


int send_file(int s, struct sockaddr* peer, int peerlen, char* userid){
	FILE * file;
	int rc;
	struct file_info fileinfo;

	char dir[100] = "database/sync_dir_";
	strcat(dir, userid);
	strcat(dir,"/");

	//printf("DEBUG: Entrou na send_files\n");

	rc = recvfrom(s, &fileinfo, sizeof(struct file_info), 0, (struct sockaddr*) peer, (socklen_t *) &peerlen);
	//printf("DEBUG FILE_INFO\nName: %s\nExt: %s\nLast Modified: %s\nSize: %d\n",fileinfo.name,fileinfo.extension,fileinfo.last_modified,fileinfo.size);

	strcat(dir,fileinfo.name);
	strcat(dir,fileinfo.extension);
	file = fopen(dir, "r");
	if (file == NULL){
			return ERROR;
	}
	//fprintf(stderr,"DEBUG: abriu arquivo send_file\n");

	struct stat st;
	stat(dir, &st);
	int length = st.st_size;
	//fileinfo.size = length;
	fileinfo.size = length;

	//printf("Size: %d\n", length);

	rc = sendto(s, &fileinfo, sizeof(struct file_info), 0, (struct sockaddr*) peer, peerlen);

	//fprintf(stderr,"DEBUG: ACK enviado send_files\n");


	struct datagram pkg = {1,1};
	strcpy(pkg.username, userid);

	while(fread(pkg.buffer,sizeof(char),BUFFER_SIZE,file)) {
			//Envia o 'pkg.buffer'
			//Bloqueia até receber o ack
			//Quando receber o ack, continua no 'while'
	}

	//fprintf(stderr,"BUFFER ARQUIVO: %s\n",pkg.buffer);

	do {
	// envia o file_info
		//fprintf(stderr, "Entrou no while\n");
		rc = sendto(s, &pkg, sizeof(struct datagram), 0, (struct sockaddr*) peer, peerlen);
	// recebe datagrama com ACK
		//rc = recvfrom(s, &pkg, sizeof(struct datagram), 0, (struct sockaddr*) peer, (socklen_t *) &peerlen);
		//printf("PKG.ID: %d\n\n", pkg.id);


	}while (rc < 0); //while (rc < 0 || pkg.id != 2 ); // recebe algo e recebe o ACK do servidor


	fclose(file);

	fprintf(stderr,"DEBUG: Saindo da função send_file\n");
	fprintf(stderr,"\n");



}

/*********************************************
*	FUNÇÕES AUXILIARES DO SERVIDOR
**********************************************/

void create_path(char *user){
printf("%s\n", user);
	struct stat st = {0};
	char dir[50] = "database/sync_dir_";
	strcat(dir, user);

	if(mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0)
		printf("Pasta %s criada.\n", dir);
	else printf("Pasta %s já existe.\n",dir);

	return;
}

int desassembly_client_inst(int word) {
	int instruction = word & 0xffff0000;

	if ( (instruction & 0xf8000000) == 0x08000000 ) {
		return ESTABLISH_CONNECTION;
	}

	if ( (instruction & 0xf8000000) == 0x10000000 ) {
		return UPLOAD;
	}

	if ( (instruction & 0xf8000000) == 0x18000000 ) {
		return DOWNLOAD;
	}

	if ( (instruction & 0xf8000000) == 0x20000000 ) {
		return LIST_SERVER;
	}

	if ( (instruction & 0xf8000000) == 0x28000000 ) {
		return LIST_CLIENT;
	}

	if ( (instruction & 0xf8000000) == 0x30000000 ) {
		return GET_SYNC_DIR;
	}

	if ( (instruction & 0xf8000000) == 0x38000000 ) {
		return EXIT;
	}

	if ( (instruction & 0xf8000000) == 0x40000000 ) {
		return BACKUP1;
	}

	if ( (instruction & 0xf8000000) == 0x48000000 ) {
		return BACKUP2;
	}

	return ERROR;
}


int assembly_server_inst(int *instruction, int instruction_id) {

	if (instruction_id == ACK) {
		*(instruction) = *(instruction) | 0x00000001; // coloca o 1 no início
		return SUCCESS;
	}

	if (instruction_id == CLEAR_INSTRUCTION_BYTE) {
		*(instruction) = *(instruction) & 0x00000000; // coloca o 1 no início
		return SUCCESS;
	}

	// LOGIN_SERVER
	if (instruction_id == CONNECTED) {
		*(instruction) = *(instruction) & 0xffff0000; // zera os LSBs
		*(instruction) = *(instruction) | 0x00000900; // add instrucao
		return SUCCESS;
	}

	if (instruction_id == FIRST_TIME_USER) {
		*(instruction) = *(instruction) & 0xffff0000; // zera os LSBs
		*(instruction) = *(instruction) | 0x00000a00; // add instrucao
		return SUCCESS;
	}

	if (instruction_id == TOO_MANY_DEVICES) {
		*(instruction) = *(instruction) & 0xffff0000; // zera os LSBs
		*(instruction) = *(instruction) | 0x00000b00; // add instrucao
		return SUCCESS;
	}

	if (instruction_id == TOO_MANY_USERS) {
		*(instruction) = *(instruction) & 0xffff0000; // zera os LSBs
		*(instruction) = *(instruction) | 0x00000c00; // add instrucao
		return SUCCESS;
	}

	// EXIT

	if (instruction_id == TERMINATE_CLIENT_EXECUTION) {
		*(instruction) = *(instruction) & 0xffff0000; // zera os LSBs
		*(instruction) = *(instruction) | 0x00003800; // add instrucao
		return SUCCESS;
	}

	// UPLOAD

	if (instruction_id == START_SENDING) {
		*(instruction) = *(instruction) & 0xffff0000; // zera os LSBs
		*(instruction) = *(instruction) | 0x00001100; // add instrucao + status
		return SUCCESS;
	}

	// DOWNLOAD
	if (instruction_id == START_DOWNLOAD) {
		*(instruction) = *(instruction) & 0xffff0000; // zera os LSBs
		*(instruction) = *(instruction) | 0x00001800; // add instrucao + status
		return SUCCESS;
	}

	return ERROR;
}

int has_too_many_devices(char username[]) {

	struct client c = get_client(username);

	for(int i=0; i<MAXDEVICES; i++) {
		if( c.devices[i] == 0){
			return FALSE;
		}
	}
	return TRUE;
}

// FUNÇÕES DE CONTROLE

struct client get_client(char username[]) {
	struct client c;
	for(int i=0; i<MAXUSERS; i++){
		if( strcmp(clients[i].userid, username) == 0 )
			c = clients[i];
	}
	return c;
}

int get_client_index(char username[]) {

	for(int i=0; i<MAXUSERS; i++){
		if( strcmp(clients[i].userid, username) == 0 )
			return i;
	}
	return ERROR;
}

int is_first_connection(char username[]) {

	if( get_client_index(username) == -1 )
		return TRUE;
	return FALSE;
}

int log_device(char username[]) {

	int index = get_client_index(username);

	for(int i=0; i<MAXDEVICES; i++) {
		if(clients[index].devices[i] == 0) {
			clients[index].devices[i] = 1;
			return index;
		}
	}
	return ERROR;
}

int log_off_device(char username[]){
	int index = get_client_index(username);

	for(int i=MAXDEVICES-1; i>=0; i--) {
		if(clients[index].devices[i] == 1){
			clients[index].devices[i] = 0;
			return index;
		}
	}
	return ERROR;
}

int save_clients() {
    FILE * out_clients;
    out_clients = fopen ("clients.dat", "w");
    if (out_clients == NULL) {
        return ERROR;
    }

    int i;
    for(i=0;i<MAXUSERS;i++) {
        fwrite (&clients[i], sizeof(struct client), 1, out_clients);
    }

    fclose(out_clients);

    if(fwrite == 0)
        return ERROR;

    return SUCCESS;
}

int load_clients() {
    FILE * in_clients;

    in_clients = fopen ("clients.dat", "r");
    if (in_clients == NULL){
        return ERROR;
    }

    int i;
    for(i=0;i<MAXUSERS;i++) {
        fread(&clients[i], sizeof(struct client), 1, in_clients);
    }

    fclose(in_clients);

    return SUCCESS;
}
