#ifndef CONNECTION_MANAGER_H_
#define CONNECTION_MANAGER_H_

//init select
fd_set master;		// master file descriptor list
fd_set read_fds;	// temp file descriptor list for select()
int fdmax;				// maximum file descriptor number

int control_socket, router_socket, data_socket;

void init();


#endif
