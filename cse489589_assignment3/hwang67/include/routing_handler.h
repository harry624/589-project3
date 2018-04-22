#ifndef ROUTING_HANDLER_H_
#define ROUTING_HANDLER_H_

#include "../include/control_handler.h"

struct __attribute__((__packed__)) ROUTING_UPDATE_HEADER
{
    uint16_t num_fields;
    uint16_t sourceRouterPort;
    uint32_t sourceIP;
};

struct __attribute__((__packed__)) ROUTING_UPDATE_ROUTER
{
    uint32_t router_IP;
    uint16_t port;
    uint16_t padding;
    uint16_t router_ID;
    uint16_t cost;
};

void update_routing(int sockfd, int neighbors[], struct Router routers[]);


#endif
