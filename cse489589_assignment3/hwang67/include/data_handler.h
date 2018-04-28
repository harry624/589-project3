#ifndef DATA_HANDLER_H_
#define DATA_HANDLER_H_


void create_data_socket(uint16_t dataPort);

int new_data_conn(int sock_index);

int isData(int sock_index);
int data_recv_hook(int sock_index);

#endif
