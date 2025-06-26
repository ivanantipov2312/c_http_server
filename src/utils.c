#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>

#define MAX_RESPONSE_LEN 1024

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
		return &((struct sockaddr_in *)sa)->sin_addr;
	return &((struct sockaddr_in6 *)sa)->sin6_addr;
}

void send_file(int client_fd, const char *status_line, const char *filename)
{
        char resp[MAX_RESPONSE_LEN];

        FILE *fp = fopen(filename, "rb");
        if (fp == NULL) {
                fprintf(stderr, "Failed to open file '%s'\n", filename);
                return;
        }

        fseek(fp, 0, SEEK_END);
        long file_len = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        if (file_len == 0) {
                fprintf(stderr, "File is empty!\n");
                return;
        }

        char *contents = malloc(file_len + 1);
        fread(contents, 1, file_len, fp);
        contents[file_len] = '\0';

        int resp_len = sprintf(resp, "%s\r\nContent-Type: text/html\r\nContent-Length: %ld\r\n\r\n%s", status_line, file_len, contents);
        send(client_fd, resp, resp_len, 0);
}
