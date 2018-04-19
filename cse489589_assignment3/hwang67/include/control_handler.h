
#ifndef CONTROL_HANDLER_H_
#define CONTROL_HANDLER_H_

int create_control_socket();
int new_control_conn(int sock_index);

int isControl(int sock_index);


#endif
