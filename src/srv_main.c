/*
	HTTP Server
	Author: Gorovoi Aleksandr
*/
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
//Consts
#define MAX_CONNECTIONS	10
#define WORKER_COUNT 10
#define DEFAULT_CONTENT_TYPE "text/html"
#define SERVER_NAME "Custom Server"
#define PROTOCOL "HTTP/1.1"
#define HOME_DIR "www"
//Home directory
char* homeDirectory;
//Thread workers pool
pthread_t workersPool[WORKER_COUNT];
//Connections
int connectionStack[MAX_CONNECTIONS];
int stackPointer = 0;
pthread_mutex_t connectionStackLocker;

char* generate404Response(){
	char headers[200];
	sprintf(headers, "%s 404 Not Found\r\nServer: %s\r\nContent-Type: text/html\r\nContent-Length: %d\r\nConnection: close\r\n\r\n",
	PROTOCOL, SERVER_NAME, 0);
	char *result = malloc(strlen(headers));
	strcpy(result, headers);
	return result;
}

char* getMIME(char* fileName){
	char command[50];
        sprintf(command, "file --mime-type -b %s", fileName);
        FILE *fp = popen(command, "r");
        if (fp != NULL){
                char buffer[200];
                fgets(buffer, sizeof(buffer), fp);
                char *result = malloc(strlen(buffer));
		strcpy(result, buffer);
		result[strlen(result)-1] = 0;
		return result;
        }
	return DEFAULT_CONTENT_TYPE;
}

char* generateOKResponse(int contentSize, char* contentType){
	char headers[200];
	sprintf(headers, "%s 200 OK\r\nServer: %s\r\nContent-Type: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n",
	PROTOCOL,SERVER_NAME, contentType, contentSize);
	char *result = malloc(strlen(headers)+1);
	strcpy(result, headers);
    	return result;
}

void connectionProcessing(int connDescr){
	char buffer[256];
        bzero(buffer, sizeof(buffer));
	//Parse request
	FILE *conn = fdopen(connDescr, "r+");
	char requestType[5];
	char *url;
	int nowLine = 0, ptr = 0, emptyStr = 0;
        while(1){
		fgets(buffer, sizeof(buffer), conn);
		ptr = 0;
		if (nowLine == 0){
			//Parse request type
			do {
				requestType[ptr]=buffer[ptr];
			} while (buffer[++ptr]!=' ');
			//Parse url
			int urlLength = (int)strlen(buffer)-(int)strlen(requestType)-(int)strlen(PROTOCOL)-4;
			url = malloc((size_t)urlLength);
			ptr++;
			do {
				url[ptr-strlen(requestType)-1]=buffer[ptr];
			} while (buffer[++ptr]!=' ');
		} else {
			//Other parameters
//			printf("%s", buffer);
		}
		nowLine++;
		if (strcmp(buffer, "\r\n") == 0){
			break;
		}
	}
	//Add home directory to URL
	char *fileName = malloc(strlen(url)+strlen(homeDirectory)+1);
	strcpy(fileName, homeDirectory);
	strcat(fileName, url);
	//Try to open file
	if (!strcmp(requestType, "GET")){
		if (fileName[strlen(fileName) - 1] == '/'){
			//Directory request - add index.html
			strcat(fileName, "index.html");
		}
		FILE *respContent = fopen(fileName, "rb");
		if (respContent != NULL){
			fseek(respContent, 0, SEEK_END);
			int contentSize = ftell(respContent);
			rewind(respContent);
			char *response = generateOKResponse(contentSize, getMIME(fileName));
			fputs(response, conn);
			char *respBuffer;
			respBuffer = (char*) malloc (sizeof(char)*contentSize);
		        size_t result = fread(respBuffer, 1, contentSize, respContent);
			fwrite(respBuffer, 1, contentSize, conn);
			fclose(respContent);
		} else {
			char *response = generate404Response();
			fprintf(conn, "%s", response);
		}
	}
	fclose(conn);
	free(fileName);
	free(url);
}

void* workerMethod(void *arg){
	int currentTask = 0;
	int lastTask = 0;
	while (1){
		currentTask=0;
		//Check task
		pthread_mutex_lock(&connectionStackLocker);
		if (stackPointer){
			currentTask = connectionStack[--stackPointer];
		}
		pthread_mutex_unlock(&connectionStackLocker);
		//Process task
		if (currentTask){
			connectionProcessing(currentTask);
			lastTask = currentTask;
		}
	}
}

void* socketProcessor(void *arg){
	int socketDescr = *((int *) arg);
	struct sockaddr_in serv_addr;
        //Start listening
        listen(socketDescr, MAX_CONNECTIONS);
        while(1){
                //Accept connection
                int connfd = accept(socketDescr, (struct sockaddr*)NULL, NULL);
                //Put connection to task
		pthread_mutex_lock(&connectionStackLocker);
                connectionStack[stackPointer++] = connfd;
		pthread_mutex_unlock(&connectionStackLocker);
        }
}

int main(int argc, char* argv[]){
	//iterators
	int i = 0;
	//Init mutex
	if (pthread_mutex_init(&connectionStackLocker, NULL) != 0){
		printf("Error while mutex initialization");
	}
	//Generate worker threads
	for (i = 0; i < WORKER_COUNT; i++){
		pthread_create(&workersPool[i], NULL, &workerMethod, NULL);
	}
	//default values
	char port[4] = "8080";
	homeDirectory = malloc(strlen(HOME_DIR));
	strcpy(homeDirectory, HOME_DIR);
	for (i = 1; i < argc; i++){
		if (!strcmp(argv[i], "-p")){
			if (i + 1 < argc){
				strcpy(port, argv[i+1]);
				printf("\nSet port to %s\n", port);
			} else {
				printf("\nMissing argument for -p\n");
			}
		}
		if (!strcmp(argv[i], "-h")){
			if (i+1<argc){
				strcpy(homeDirectory, argv[i+1]);
				printf("\nSet home directory to %s\n", homeDirectory);
			} else {
				printf("\nMissing argument for -h\n");
			}
		}
	}
	//Parse arguments
	printf("\nStart server on port %s...\n", port);
	printf("\nHome directory is %s\n", homeDirectory);
	//Socket binding
	int sockfd;
	if ((sockfd  = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("\nError while socket opening\n");
		return 1;
	}
	struct sockaddr_in serv_addr;
	//Clear
	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(port));
	printf("\nSocket binding...\n");
	if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
		printf("\nError while socket binding\n");
		return 1;
	}
	//Socket processing
	pthread_t socketProcessorThread;
	pthread_create(&socketProcessorThread, NULL, &socketProcessor, &sockfd);
	//Server started!
	printf("Server started!\n");
	//Waiting for commands
	while (1){
		char command[100];
		printf("httpserver $>");
		scanf("%s", command);
		if (!strcmp(command, "\\q")){
			close(sockfd);
			return 0;
		} else if (!strcmp(command, "\\cl")){
			printf("\nConnection list:");
			for (i = 0; i < stackPointer; i++){
				printf(" %d", connectionStack[i]);
			}
			printf("\n");
		} else {
			printf("\nCommand not supported\n");
		}
	}
	return 0;
}
