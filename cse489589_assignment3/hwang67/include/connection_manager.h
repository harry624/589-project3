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
fd_set read_fds;	// temp file descriptor list for select()
int fdmax;				// maximum file descriptor number

//timer
int timerStart;
int timerEnd;

struct timer{
		int timerNodeIndex;
		struct timeval time;
		int isUpdatedEarly;
};

struct timer timerArray[5];


int control_socket, router_socket, data_socket;

void init();


#endif
