#ifndef ROUTING_HANDLER_H_
#define ROUTING_HANDLER_H_

#include "../include/control_handler.h"

#define ROUTING_HEADER_SIZE 8

struct __attribute__((__packed__)) ROUTING_UPDATE_HEADER
{
    uint16_t num_fields;
    uint16_t sourceRouterPort;
    uint32_t sourceIP;
};

struct __attribute__((__packed__)) ROUTING_UPDATE_ROUTER
{
    uint32_t routerIP;
    uint16_t port;
    uint16_t padding;
    uint16_t routerID;
    uint16_t cost;
};

int new_routing_conn(int sock_index);

void recv_update_distanceVector(int sockfd);

void boardcast_update_routing(int sockfd, int neighbors[], struct Router routers[]);


#endif
