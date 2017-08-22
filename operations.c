#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>

#include "operations.h"

#define BUFFER_SIZE 1024

void badRequest(int fd)
{
	printf("Bad Request\n");
	send(fd, "HTTP/1.0 400 Bad Request\r\n",26, MSG_DONTWAIT);
	send(fd, "Content-Type: text.html\r\nContent-Length: 32\r\n\r\n", 47, MSG_DONTWAIT);
	send(fd, "<h1>Error 400 Bad Request</h1>\r\n", 32, MSG_DONTWAIT);
	exit(-1);
}
void permissionDenied(int fd)
{
	printf("Permission Denied\n");
	send(fd, "HTTP/1.0 403 Permission Denied\r\n",32, MSG_DONTWAIT);
	send(fd, "Content-Type: text.html\r\nContent-Length: 38\r\n\r\n", 47, MSG_DONTWAIT);
	send(fd, "<h1>Error 403 Permission Denied</h1>\r\n", 38, MSG_DONTWAIT);
	exit(-1);
}
void notFound(int fd)
{
	printf("Not Found\n");
	send(fd, "HTTP/1.0 404 Not Found\r\n", 24, MSG_DONTWAIT);
	send(fd, "Content-Type: text.html\r\nContent-Length: 30\r\n\r\n", 47, MSG_DONTWAIT);
	send(fd, "<h1>Error 404 Not Found</h1>\r\n", 30, MSG_DONTWAIT);
	exit(-1);
}
void internalError(int fd)
{
	printf("internal Error\n");
	send(fd, "HTTP/1.0 500 Internal Error\r\n", 29, MSG_DONTWAIT);
	send(fd, "Content-Type: text.html\r\nContent-Length: 35\r\n\r\n", 47, MSG_DONTWAIT);
	send(fd, "<h1>Error 500 Internal Error</h1>\r\n", 35, MSG_DONTWAIT);
	exit(-1);
}
void notImplemented(int fd)
{
	printf("not implemented\n");
	send(fd, "HTTP/1.0 501 Not Implemented\r\n", 30, MSG_DONTWAIT);
	send(fd, "Content-Type: text.html\r\nContent-Length: 36\r\n\r\n", 47, MSG_DONTWAIT);
	send(fd, "<h1>Error 501 Not Implemented</h1>\r\n", 36, MSG_DONTWAIT);
	exit(-1);
}
int verify(char * string)
{
	int x = 0;
	int len = strlen(string);
	while(x < len)
	{
		if(!isdigit(*(string + x)))
			return 1;
		++x;
	}
	return 0;
}
int checkFilename(int fd, char * path)
{
	if(strcmp(path, "/") == 0)
	{
		badRequest(fd);
	}
	else if(strstr(path, "./cgi-like") == path)
	{
		return 1;
	}
	return 0;
}
void checkDir(int fd, char * filename)
{
	struct stat temp;
	if(stat(filename, &temp) < 0)
	{
		notFound(fd);
	}
	if(S_ISDIR(temp.st_mode))
	{
		badRequest(fd);
	}
}
/*0 if not valid, 1 if valid*/
int checkValidPath(int fd, char * path)
{
	char pathcpy[sizeof(path)];
	char * temp;
	int directory = 0;
	strcpy(pathcpy, path);
	temp = strtok(pathcpy, "/");
	while(temp != NULL)
	{
		if(strcmp(temp, "..") == 0)
		{
			if(directory == 0)
			{
				badRequest(fd);
			}
			directory--;
		}
		else
		{
			directory++;
		}
		temp = strtok(NULL, "/");
	}
	return 1;
}
	
	
void handle_request(int fd)
{
	char buff[BUFFER_SIZE];
	char * filename;
	char * http;
	char completeFilename[BUFFER_SIZE];
	memset(buff, '\0', BUFFER_SIZE);
	memset(completeFilename, '\0', BUFFER_SIZE);

	if(read(fd, buff, BUFFER_SIZE) < 0)
	{
		internalError(fd);
	}

	strtok(buff, " ");
	filename = strtok(NULL, " ");
	http = strtok(NULL, " \n\r");
	strtok(NULL, " \n\r");

	strcpy(completeFilename, ".\0");
	strcat(completeFilename, filename);

	//checkHTTP(fd, http);

	printf("type: '%s'\n", buff);
	printf("filename: '%s'\n", filename);
	printf("completeFilename: '%s'\n", completeFilename);
	printf("verify: %d\n", checkValidPath(fd, filename));
	printf("http: '%s'\n", http);

	if(checkFilename(fd, completeFilename) == 1)
	{
		printf("entering cgi-like\n");
		if(strcmp(buff, "HEAD") != 0 && strcmp(buff, "GET") != 0)
		{
			notImplemented(fd);
		}
		cgi(fd, buff, completeFilename);
		exit(0);
	}

	checkDir(fd, completeFilename);
	
	if(checkValidPath(fd, filename) == 0)
	{
		permissionDenied(fd);
	} 
	
	if(strcmp(buff, "HEAD") == 0)
	{
		printf("HEAD request\n");
		header(fd, completeFilename);
	}
	else if(strcmp(buff, "GET") == 0)
	{
		printf("Get request\n");
		header(fd, completeFilename);
		printFile(fd, completeFilename);
	}
	else
	{
		notImplemented(fd);
	}
	exit(0);
}
void checkHTTP(int fd, char * http)
{
	if(strcmp(http, "HTTP/1.0") != 0)
	{
		badRequest(fd);
	}
}
void printFile(int fd, char * filename)
{
	int c;
	FILE * file;
	file = fopen(filename, "r");
	if(file == NULL)
	{
		permissionDenied(fd);
	}
	while((c = getc(file)) != EOF)
	{
		send(fd, &c, 1, MSG_DONTWAIT);
	}
	fclose(file);
}

void header(int fd, char * filename)
{
	struct stat temp;
	
	char contentLength[BUFFER_SIZE];

	memset(contentLength, '\0', BUFFER_SIZE);

	if(stat(filename, &temp) != 0)
	{
		notFound(fd);
	}
	checkHeader(fd, filename);
	sprintf(contentLength, "Content-Length: %d\r\n", (int)temp.st_size);

	send(fd, "HTTP/1.0 200 OK\r\n", 17, MSG_DONTWAIT);
	send(fd, "Content-Type: text/html\r\n", 25, MSG_DONTWAIT);
	send(fd, contentLength, strlen(contentLength), MSG_DONTWAIT);
	send(fd, "\r\n", 2, MSG_DONTWAIT);
}

void checkHeader(int fd, char * filename)
{
	struct stat temp;
	if(stat(filename, &temp) < 0)
	{
		notFound(fd);
	}
	
	if((temp.st_mode & S_IROTH) != 0 || (temp.st_mode & S_IRUSR) != 256)
	{
		permissionDenied(fd);
	}
}

void cgi(int fd, char * type, char * filename)
{
	char ** command = parsecgi(fd, filename);
	static char tempFile[100];
	pid_t pid;
	pid_t ppid = getpid();
	sprintf(tempFile, "./temp%d.txt", ppid);

	printf("%s\n", command[0]);

	checkExecute(fd, filename);
	printf("checked execute\n");
	if((pid = fork()) < 0)
	{
		internalError(fd);
	}
	if(pid == 0)
	{
		int newFile = open(tempFile, O_WRONLY|O_CREAT, S_IRUSR|S_IROTH);
		dup2(newFile, 1);
		execvp(command[0], command);
	}
	else
	{
		waitpid(pid, NULL, 0);
		header(fd, tempFile);
		printFile(fd, tempFile);
		remove(tempFile);
	}
		
}
void checkExecute(int fd, char * filename)
{
	struct stat temp;
	if(stat(filename, &temp) < 0)
	{
		notFound(fd);
	}
	if((temp.st_mode & S_IXOTH) != 1)
	{
		permissionDenied(fd);
	}
}
char ** parsecgi(int fd, char * filename)
{
	char ** result;
	int count = 0;
	char * temp = filename;

	while(*temp)
	{
		if(*temp == '?' && count > 0)
		{
			badRequest(fd);
		}
		if(*temp == '?' || *temp == '&')
		{
			count++;
		}
		temp++;
	}
	result = malloc(sizeof(char *) * (count + 1));
	if(result)
	{
		size_t idx = 0;
		char * token = strtok(filename, "?");
		while(token != NULL)
		{
			result[idx] = token;
			token = strtok(NULL, "&");
			idx++;
		}
		result[idx] = NULL;
	}
	else
	{
		free(result);
		internalError(fd);
	}
	return result;
}

void handle_sigchld(int sig)
{
  	int saved_errno = errno;
  	wait(NULL);
  	errno = saved_errno;
}
