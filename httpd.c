#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include "operations.h"
#include "simple_net.h"

#define QUEUE_SIZE 8192
int sockfd, newsockfd;

int main(int argc, char ** argv)
{
	unsigned short port;
	pid_t pid;

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = &handle_sigchld;
	if (sigaction(SIGCHLD, &sa, 0) == -1) 
	{
  		fprintf(stderr, "Sigaction Error\n");
  		exit(-1);
	}

	if(argc != 2)
	{
		fprintf(stderr, "No valid port number provided\n");
		exit(-1);
	}
	port = atoi(argv[1]);
	if((sockfd = create_service(port, QUEUE_SIZE)) < 0)
	{
		fprintf(stderr, "Cannot create service\n");
		exit(-1);
	}
	while(1)
	{
		if((newsockfd = accept_connection(sockfd)) < 0)
		{
			fprintf(stderr, "Cannot accept connection\n");
			exit(-1);
		}
		printf("\nConnection Accepted\n");
		if((pid = fork()) < 0)
		{
			fprintf(stderr, "Fork Error\n");
		}
		if(pid == 0)
		{
			handle_request(newsockfd);
		}
		close(newsockfd);
	}
	close(sockfd);
	return 0;
}
