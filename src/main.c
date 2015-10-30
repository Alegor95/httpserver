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
//Consts
#define MAX_CONNECTIONS	10

int main(int argc, char* argv[]){
	//iterators
	int i = 0;
	//default port
	char port[4] = "8000";
	for (i = 1; i < argc; i++){
		if (strcmp(argv[i], "-p")){
			if (i < argc){
				strcpy(port, argv[i]);
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
	//Start listening
	listen(sockfd, MAX_CONNECTIONS);
	//Accept connection
	int connfd = accept(sockfd, (struct sockaddr*)NULL, NULL);
	//Read info to bufer
	char buffer[256];
	bzero(buffer, sizeof(buffer));
	read(connfd,buffer, sizeof(buffer));
	printf("%s", buffer);
	//Close connection
	close(connfd);
	return 0;
}
