#include "connection.h"

int main() {
	printf("Intializing server...\n");
    int server_fd, opt = 1;
	if ((server_fd = createTCPipv4Socket()) < 0) {
		perror("socket");
		return EXIT_FAILURE;
	}
	struct sockaddr_in* addr = createIpv4Address(NULL, PORT);

    for (int i = 0; i < BACKLOG; i++) {
        acceptedSockets[i].socket_fd = -1; // mark all as free initially
    }

	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	if (bind(server_fd, (struct sockaddr*)addr, sizeof(*addr)) < 0) {
		perror("bind");
		return EXIT_FAILURE;
	}
	if (listen(server_fd, BACKLOG) < 0) {
		perror("listen");
		return EXIT_FAILURE;
	}
	printf("Server ready to accept clients...\n");
	pthread_t id = startAcceptingIncomingConnections(server_fd);
	pthread_join(id, NULL);

	printf("Closing all client sockets...\n");
	for (int i = 0; i < socketsCount; i++) {
		if (acceptedSockets[i].socket_fd >= 0) {
			close(acceptedSockets[i].socket_fd);
		}
	}

	shutdown(server_fd, SHUT_RDWR);
	free(addr);
	printf("Server closed\n");
	return EXIT_SUCCESS;
}

