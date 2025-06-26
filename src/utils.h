#ifndef UTILS_H_
#define UTILS_H_

#include <sys/socket.h>

void *get_in_addr(struct sockaddr *sa);
void send_file(int client_fd, const char *status_line, const char *filename);

#endif // UTILS_H_
