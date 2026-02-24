#include "connection.h"
#include "message.h"
#include <errno.h>

struct AcceptedSocket acceptedSockets[BACKLOG];
int socketsCount;

struct AcceptedSocket* acceptIncomingConnection(int serverSocket_fd) {
	struct sockaddr_in client_addr;
	socklen_t clientAddrSize = sizeof(client_addr);
	int client_fd = accept(serverSocket_fd, (struct sockaddr*)&client_addr, &clientAddrSize);
	struct AcceptedSocket* socket = malloc(sizeof(struct AcceptedSocket));
	socket->socket_fd = client_fd;
	socket->address = client_addr;
    char name[50], msg[100];
    int bytes = recv(client_fd, name, sizeof(name) - 1, 0);
    if (bytes > 0) {
        name[bytes] = '\0';
        if (usernameExists(name)) {
            send(client_fd, "NOTACCEPTED", 12, 0);
            socket->acceptedSuccessfully = 0;
        } else {
            send(client_fd, "ACCEPTED", 9, 0);
            snprintf(socket->name, sizeof(socket->name), "%s", name);
            snprintf(msg, sizeof(msg), BGRN"%s"GRN" joined"reset, socket->name);
            printf("%s\n", msg);
            sendMessageToOtherClients(msg, client_fd);
        }
    }
	socket->acceptedSuccessfully = client_fd >= 0;
	if (!socket->acceptedSuccessfully) socket->error = client_fd;
	return socket;
}

void* acceptNewConnectionAndPrint(void* args) {
	int server_fd = *(int*)args;
	free(args);
	while (true) {
		struct AcceptedSocket* clientSocket = acceptIncomingConnection(server_fd);
		if (!clientSocket->acceptedSuccessfully) {
			fprintf(stderr, "accept failed: %s\n", strerror(errno));
			free(clientSocket);
			continue;
		}
        int foundSlot = -1;
        for (int i = 0; i < BACKLOG; i++) {
            if (acceptedSockets[i].socket_fd < 0) { // free slot
                foundSlot = i;
                break;
            }
        }

        if (foundSlot != -1) {
            acceptedSockets[foundSlot] = *clientSocket;
            printf("Client accepted on socket %d.\n", clientSocket->socket_fd);
            socketsCount++;
            recieveAndPrintDataOnSeperateThread(clientSocket);
		} else {
			fprintf(stderr, "Max connections reached. Rejecting client.\n");
			close(clientSocket->socket_fd);
			free(clientSocket);
		}
	}
	return NULL;
}

// for accept()
pthread_t startAcceptingIncomingConnections(int server_fd) {
	int* arg = malloc(sizeof(int));
	*arg = server_fd;
	pthread_t id;
	pthread_create(&id, NULL, acceptNewConnectionAndPrint, arg);
	return id;
}

