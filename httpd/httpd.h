#ifndef _HTTPD_
#define _HTTPD_

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#define SIZE 1024

#define SUCCESS 0
#define NOTICE 1
#define WARNING 2
#define ERROR 3
#define FATAL 4

void print_log(const char* msg,int level);
int startup(const char* ip,int port);
void *hander_request(void * arg);
#endif
