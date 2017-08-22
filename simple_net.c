#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#include "simple_net.h"

int create_service(unsigned short port, int queue_size)
{
	int fd;
	struct sockaddr_in local_addr;
	int yes = 1;
	
	if((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		return -1;
	}
	
	if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
	{
		return -1;
	}

	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(port);
	local_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(local_addr.sin_zero), '\0', 8);
	
	if(bind(fd, (struct sockaddr *)&local_addr, sizeof(struct sockaddr)) == -1)
	{
		return -1;
	}
	if(listen(fd, queue_size) == -1)
	{
		return -1;
	}
	
	return fd;
}

int accept_connection(int fd)
{
	int new_fd;
	struct sockaddr_in remote_addr;
	socklen_t size = sizeof(struct sockaddr_in);
	
	errno = EINTR;
	while(errno == EINTR)
	{
		if((new_fd = accept(fd, (struct sockaddr*)&remote_addr, &size)) == -1 && errno != EINTR)
		{
			return -1;
		}
		else if(new_fd != -1)
		{
			break;
		}
	}
	return new_fd;
}
