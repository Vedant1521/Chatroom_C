#include "../utils/socketutils.h"
#include "ai.h"

#define IP "127.0.0.1"
#define START "\033[1G"

typedef enum Chat_State { PUBLIC,
						  PRIVATE,
						  AICHAT } ChatState;

char prompt[100] = BHYEL "> " reset;

void* listenPrintChatsOfOthers(void* arg) {
	int fd = *(int*)arg;
	free(arg);
	char buffer[BUF_SIZE];
	while (true) {
		ssize_t bytes_received = recv(fd, buffer, BUF_SIZE - 1, 0);
		if (bytes_received > 0) {
			buffer[bytes_received] = '\0';
			if (strncmp(buffer, "USERLIST:", 9) == 0) {
				char* users = buffer;
				users += 9;
				char* token = strtok(users, ",");
				printf(START);
				if (token == NULL)
					printf(YEL "No active users except you\n" reset);
				else
					printf("Active Users:\n");
				while (token != NULL) {
					printf("- %s\n", token);
					token = strtok(NULL, ",");
				}
				printf("\n");
			} else {
				printf(START "%s\n", buffer);
			}
			printf("%s", prompt);
			fflush(stdout);
		} else if (bytes_received == 0) {
			printf(START "Disconnected.\n");
			break;
		} else {
			perror("recv");
			break;
		}
	}
	close(fd);
	return NULL;
}

void listenOnNewThread(int fd) {
	int* arg = malloc(sizeof(int));
	*arg = fd;
	pthread_t id;
	pthread_create(&id, NULL, listenPrintChatsOfOthers, arg);
}

int setupConnection(char* ip, int port) {
	int socket_fd;
	if ((socket_fd = createTCPipv4Socket()) < 0) {
		perror("socket");
		return -1;
	}
	struct sockaddr_in* addr;
	if ((addr = createIpv4Address(ip, port)) == NULL) {
		printf("Invalid Address\n");
		close(socket_fd);
		return -1;
	}
	if (connect(socket_fd, (struct sockaddr*)addr, sizeof(*addr)) < 0) {
		perror("connect");
		close(socket_fd);
		return -1;
	}
	return socket_fd;
}

char* getUserName() {
	char* name = NULL;
	size_t nameSize = 0;
	printf("Enter you name: ");
	ssize_t byteCount = getline(&name, &nameSize, stdin);
	if (byteCount > 0) {
		name[byteCount - 1] = '\0';
	}
	return name;
}

void displayHelp() {
	printf("Commands:\n");
	printf("  /help - show this message\n");
	printf("  /quit - quit the application\n");
	printf("  /users - list all active users\n");
	printf("  /chat <username> - to enter private chat with a specific user\n");
	printf("  /exit - to exit the private chat\n");
	printf("  /ai - to chat with ai\n");
}

void handleChatCommand(ChatState* state, char* privateChatRecipient, char* line) {
	if (*state == PRIVATE) {
		printf("You are in already private chat with someone\n");
	} else {
		*state = PRIVATE;
		strcpy(privateChatRecipient, line + 6);
		sprintf(prompt, CYN "(to " BCYN "%s" CYN ")> " reset, privateChatRecipient);
	}
}

void handleExitCommand(ChatState* state) {
	if (*state == PUBLIC) {
		printf("You are already in public chat\n");
	} else {
		printf("Exited the %s chat\n", (*state == PRIVATE) ? "private" : "ai");
		*state = PUBLIC;
		sprintf(prompt, BHYEL "> " reset);
	}
}

void handleAIchat(ChatState* state) {
	if (*state == AICHAT) {
		printf("You are already in ai chat\n");
		return;
	}
	*state = AICHAT;
	sprintf(prompt, BHBLU "(ai)> " reset);
}

bool handleCommand(char* line, int socket_fd, ChatState* state, char* privateChatRecipient) {
	if (strcmp(line, "/quit") == 0)
		return true;
	else if (strcmp(line, "/help") == 0)
		displayHelp();
	else if (strcmp(line, "/users") == 0)
		send(socket_fd, line, strlen(line), 0);
	else if (strncmp(line, "/chat ", 6) == 0)
		handleChatCommand(state, privateChatRecipient, line);
	else if (strcmp(line, "/exit") == 0)
		handleExitCommand(state);
	else if (strcmp(line, "/ai") == 0)
		handleAIchat(state);
	else
		printf("Unknown command: %s\n", line);
	return false;
}

void sendMessage(int socket_fd, char* name, char* line, ChatState state, char* privateChatRecipient) {
	char buffer[BUF_SIZE];
	sprintf(buffer, BBLU "%s:" reset " %-10s", name, line);
    if (state == AICHAT) {
        printf(START "%s\n", response(line));
    } else if (state == PRIVATE) {
		char prototype[BUF_SIZE + 100];
		sprintf(prototype, "PVTMSG %s %s", privateChatRecipient, buffer);
		send(socket_fd, prototype, strlen(prototype), 0);
		char validation[10];
		recv(socket_fd, validation, 10, 0);
		if (strcmp(validation, "NOTFOUND")) {
			printf(START "No such user\n");
		} else
			printf(START "Server responded: %s\n", validation);
	} else {
		send(socket_fd, buffer, strlen(buffer), 0);
	}
}

void chatLoop(int socket_fd, char* name) {
	ChatState currentState = PUBLIC;
	char privateChatRecipient[50];
	char* line = NULL;
	size_t lineSize = 0;
	while (true) {
		printf("%s", prompt);
		fflush(stdout);
		ssize_t charCount = getline(&line, &lineSize, stdin);
		if (charCount <= 0) continue;
		if (line[charCount - 1] == '\n') {
			line[charCount - 1] = '\0';
		}
		if (line[0] == '/') {
			if (handleCommand(line, socket_fd, &currentState, privateChatRecipient)) break;
		} else
			sendMessage(socket_fd, name, line, currentState, privateChatRecipient);
	}
	free(line);
}

int main() {
	int socket_fd = setupConnection(IP, PORT);
	if (socket_fd < 0) {
		return EXIT_FAILURE;
	}
	char* name = getUserName();
	if (name == NULL) {
		close(socket_fd);
		return EXIT_FAILURE;
	}
	char approval[15];
	send(socket_fd, name, strlen(name), 0);
	int bytes = recv(socket_fd, approval, 15, 0);
	if (bytes <= 0) printf("Server didn't respond\n");
	if (strcmp(approval, "ACCEPTED")) {
		printf("Username already exists\n");
		free(name);
		close(socket_fd);
		return EXIT_FAILURE;
	}

	printf("Welcome " BMAG "%s" reset " to my chat app.. (Enter /help for commands)\n", name);

	listenOnNewThread(socket_fd);
	chatLoop(socket_fd, name);

	free(name);
	close(socket_fd);
	return EXIT_SUCCESS;
}
