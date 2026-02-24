#include "socketutils.h"

int createTCPipv4Socket() {
	return socket(AF_INET, SOCK_STREAM, 0);
}

struct sockaddr_in* createIpv4Address(char* ip, int port) {
	struct sockaddr_in* address = malloc(sizeof(struct sockaddr_in));
	address->sin_family = AF_INET;
	address->sin_port = htons(port);
	if (ip == NULL) {
		address->sin_addr.s_addr = INADDR_ANY;
	} else {
		if ((inet_pton(AF_INET, ip, &address->sin_addr)) < 0) 
			return NULL;
	}
	return address;
}
