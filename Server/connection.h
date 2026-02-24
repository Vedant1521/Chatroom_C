#ifndef CONNECTION_H
#define CONNECTION_H

#include "../utils/socketutils.h"

struct AcceptedSocket {
    int socket_fd;
    struct sockaddr_in address;
    int error;
    bool acceptedSuccessfully;
    char name[50];
};

extern struct AcceptedSocket acceptedSockets[BACKLOG];
extern int socketsCount;

struct AcceptedSocket* acceptIncomingConnection(int serverSocket_fd);
void* acceptNewConnectionAndPrint(void* args);
pthread_t startAcceptingIncomingConnections(int server_fd);

#endif
