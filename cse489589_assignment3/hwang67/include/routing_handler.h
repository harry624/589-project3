#ifndef ROUTING_HANDLER_H_
#define ROUTING_HANDLER_H_

#include <time.h>
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

void create_router_socket(uint16_t routerPort);

int create_UDP_listener_socket(uint16_t router_port);

int recv_update_distanceVector(int sockfd);

void boardcast_update_routing(int sockfd, int neighbors[], struct Router routers[]);

void updateDVBybellmanFord();

#endif
