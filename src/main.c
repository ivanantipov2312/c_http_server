#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#include "tpool.h"
#include "utils.h"

bool get_listener_socket(const char *port, int backlog, int *sock_fd)
{
	struct addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	struct addrinfo *res;
	int status = getaddrinfo(NULL, port, &hints, &res);
	if (status != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return false;
	}

	struct addrinfo *p;
	for (p = res; p != NULL; p = p->ai_next) {
		*sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (*sock_fd == -1) {
			perror("socket");
			continue;
		}

		int yes = 1;
		if (setsockopt(*sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt");
			close(*sock_fd);
			continue;
		}

		if (bind(*sock_fd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("bind");
			close(*sock_fd);
			continue;
		}

		break;
	}

	freeaddrinfo(res);
	if (p == NULL) {
		fprintf(stderr, "No addresses.\n");
		return false;
	}

	if (listen(*sock_fd, backlog) == -1) {
		perror("listen");
		close(*sock_fd);
		return false;
	}

	return true;
}

void *handle_response(void *arg)
{
	int client_fd = *(int *)arg;
	free(arg);

	send(client_fd, "Hello\n", 6, 0);
	close(client_fd);

	return NULL;
}

int main(void)
{
	int sock_fd = -1;
	const char *port = "8080";
	int backlog = 128;

	struct tpool *pool = tpool_create(4);
	if (pool == NULL)
		return -1;

	if (!get_listener_socket(port, backlog, &sock_fd))
		return -1;

	printf("Waiting for connections on port %s with backlog %d...\n", port, backlog);

	while (1) {
		struct sockaddr_storage client_addr;
		socklen_t addr_len = sizeof client_addr;

		int *client_fd = malloc(sizeof(int));
		*client_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &addr_len);
		if (*client_fd == -1) {
			perror("accept");
			continue;
		}

		char ipstr[INET6_ADDRSTRLEN];
		inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), ipstr, sizeof ipstr);
		printf("Got connection from %s\n", ipstr);
	
		tpool_add_work(pool, handle_response, client_fd);
	}

	tpool_destroy(pool);
	close(sock_fd);
	return 0;
}
