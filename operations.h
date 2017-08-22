#ifndef OPERATIONS_H
#define OPERATIONS_H

void badRequest(int fd);
void permissionDenied(int fd);
void notFound(int fd);
void internalError(int fd);

int verify(char * string);
int checkValidPath(int fd, char * path);
void handle_request(int fd);
void checkHTTP(int fd, char * http);
void printFile(int fd, char * filename);
void header(int fd, char * filename);
void cgi(int fd, char * type, char * filename);
void checkExecute(int fd, char * filename);
char ** parsecgi(int fd, char * filename);
void handle_sigchld(int sig);
void checkHeader(int fd, char * filename);

#endif
