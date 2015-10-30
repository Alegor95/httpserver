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

void* threadProcessor(void *arg){
	int connDescr = *((int *) arg);
	char buffer[256];
        bzero(buffer, sizeof(buffer));
        read(connDescr,buffer, sizeof(buffer));
        printf("%s", buffer);
}

void* socketProcessor(void *arg){
	int socketDescr = *((int *) arg);
	struct sockaddr_in serv_addr;
        //Start listening
        listen(socketDescr, MAX_CONNECTIONS);
        while(1){
                //Accept connection
                int connfd = accept(socketDescr, (struct sockaddr*)NULL, NULL);
                //Read info to bufer
                threadProcessor(&connfd);
                //Close connection
                close(connfd);
        }
}

int main(int argc, char* argv[]){
	//iterators
	int i = 0;
	//default port
	char port[4] = "8080";
	for (i = 1; i < argc; i++){
		if (!strcmp(argv[i], "-p")){
			if (i + 1 < argc){
				strcpy(port, argv[i+1]);
				printf("Set port to %s", port);
			} else {
				printf("\nMissing argument for -p\n");
			}
		}
	}
	//Parse arguments
	printf("\nStart server on port %s...\n", port);
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
	bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	//Socket processing
	pthread_t socketProcessorThread;
	pthread_create(&socketProcessorThread, NULL, &socketProcessor, &sockfd);
	//Waiting for commands
	while (1){
		char command[100];
		printf("httpserver $>");
		scanf("%s", command);
		if (!strcmp(command, "\\q")){
			close(sockfd);
			return 0;
		} else {
			printf("\nCommand not supported\n");
		}
	}
	return 0;
}
