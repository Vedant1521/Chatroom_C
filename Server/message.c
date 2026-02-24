#include "connection.h"
#include "message.h"

char *getUsernameByFd(int fd) {
    char *user = NULL;
    for (int i = 0; i < BACKLOG; i++) {
        if (acceptedSockets[i].socket_fd == fd) {
            user = acceptedSockets[i].name;
            break;
        }
    }
    return user;
}

bool usernameExists(char *name) {
    for (int i = 0; i < BACKLOG; i++) {
        if (acceptedSockets[i].socket_fd != -1 && strcmp(acceptedSockets[i].name, name) == 0)
            return true;
    }
    return false;
}

void handlePrivateChats(char *message, int sender_fd) {
    char chat[BUF_SIZE];
    char recipientName[50];
    int reciever_fd = -1;
    char *senderName = getUsernameByFd(sender_fd);
    if (sscanf(message, "PVTMSG %49s %[^\n]", recipientName, chat) == 2) {
        char *dm = chat;
        dm += strlen(senderName) + strlen(BBLU) + strlen(reset) + 2;
        char buffer[BUF_SIZE + 100];
        sprintf(buffer, CYN"(private chat)"BCYN" %s:"reset" %s", senderName, dm);
        for (int i = 0; i < BACKLOG; i++) {
            if (acceptedSockets[i].socket_fd != -1 && strcmp(acceptedSockets[i].name, recipientName) == 0) {
                reciever_fd = acceptedSockets[i].socket_fd;
            }
        }
        if (reciever_fd == -1) {
            send(sender_fd, "NOTFOUND", 9, 0);
            printf("reciever not found\n");
            return;
        }
        send(sender_fd, "FOUND", 6, 0);
        send(reciever_fd, buffer, strlen(buffer), 0);
    } else {
        printf("parsing error\n");
    }
}

void sendMessageToOtherClients(char* message, int socket_fd) {
	for (int i = 0; i < BACKLOG; i++) {
		if (acceptedSockets[i].socket_fd == socket_fd || acceptedSockets[i].socket_fd < 0) continue;
		send(acceptedSockets[i].socket_fd, message, strlen(message), 0);
	}
}

void handle_command(char *buffer, int fd) {
    if (strcmp(buffer, "/users") == 0) {
        char final_response[BUF_SIZE] = "USERLIST:";
        for (int i = 0; i < BACKLOG; i++) {
            if (acceptedSockets[i].socket_fd == -1 || acceptedSockets[i].socket_fd == fd) continue;
            strcat(final_response, acceptedSockets[i].name);
            strcat(final_response, ",");
        }
        send(fd, final_response, strlen(final_response), 0);
    }
}

void* recieveAndPrintData(void* args) {
	int socket_fd = *(int*)args;
	free(args);
	char buffer[BUF_SIZE];
	while (true) {
		ssize_t bytes_received = recv(socket_fd, buffer, BUF_SIZE - 1, 0);
		if (bytes_received > 0) {
			buffer[bytes_received] = '\0';
            if (buffer[0] == '/') {
                handle_command(buffer, socket_fd);
            } else if (strncmp(buffer, "PVTMSG", 6) == 0) {
                handlePrivateChats(buffer, socket_fd);
            } else {
                printf("%s\n", buffer);
                sendMessageToOtherClients(buffer, socket_fd);
            }
		} else if (bytes_received == 0) {
			printf("Client disconnected.\n");
            socketsCount--;
			break;
		} else {
			perror("recv");
			break;
		}
	}
	for (int i = 0; i < BACKLOG; i++) {
		if (acceptedSockets[i].socket_fd == socket_fd && !usernameExists(acceptedSockets[i].name)) {
			acceptedSockets[i].socket_fd = -1;
            char leftMsg[100];
            sprintf(leftMsg, BRED"%s"RED" left the chat"reset, acceptedSockets[i].name);
            printf("%s\n", leftMsg);
            sendMessageToOtherClients(leftMsg, socket_fd);
			break;
		}
	}
	close(socket_fd);
	return NULL;
}

// worker threads
void recieveAndPrintDataOnSeperateThread(struct AcceptedSocket* pSocket) {
	pthread_t id;

	if (!pSocket->acceptedSuccessfully) {
		free(pSocket);
		return;
	}
	int* arg = malloc(sizeof(int));
	*arg = pSocket->socket_fd;
	free(pSocket);
	pthread_create(&id, NULL, recieveAndPrintData, arg);
}
