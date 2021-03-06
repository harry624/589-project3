#ifndef CONTROL_HANDLER_H_
#define CONTROL_HANDLER_H_


int distanceVector[5][5];
int neighbors[5];
int myNeighborHop[5];
uint16_t num_neighbors;
uint16_t updated_num_neighbors;
uint16_t boardcast_interval;


struct __attribute__((__packed__)) INIT_HEADER
{
    uint16_t num_neighbors;
    uint16_t update_interval;

};

int create_control_socket();
int new_control_conn(int sock_index);

int isControl(int sock_index);
int control_recv_hook(int sock_index);

#endif
