#include <arpa/inet.h>
#include <pthread.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "colors.h"

#define BUF_SIZE 1024
#define PORT 7089

#define BACKLOG 10

int createTCPipv4Socket();
struct sockaddr_in* createIpv4Address(char *ip, int port);
