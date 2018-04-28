#ifndef CONNECTION_MANAGER_H_
#define CONNECTION_MANAGER_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

//init select
fd_set master;		// master file descriptor list
int fdmax;				// maximum file descriptor number

int control_socket, router_socket, data_socket;

void init();


#endif
