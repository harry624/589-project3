#ifndef NETWORK_UTIL_H_
#define NETWORK_UTIL_H_

ssize_t sendtoALL(int sock_index, char *buffer, ssize_t nbytes, struct sockaddr_in to);
ssize_t recvfromALL(int sock_index, char *buffer, ssize_t nbytes);

ssize_t recvALL(int sock_index, char *buffer, ssize_t nbytes);
ssize_t sendALL(int sock_index, char *buffer, ssize_t nbytes);

#endif
