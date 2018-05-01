#ifndef GLOBAL_H_
#define GLOBAL_H_

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>


uint16_t CONTROL_PORT;
#define BACKLOG 5
#define INF 65535
int localRouterID;

struct Router
{
    uint16_t routerID;
    uint16_t routerPort;
    uint16_t dataPort;
    uint16_t cost;
    uint16_t nextHopID;

    char ipAddress[40];
    int UDPsockfd;
    int firstupdateReceived;
    int missedcnt;
    int counter;
};

struct Router routers[5];

/* https://scaryreasoner.wordpress.com/2009/02/28/checking-sizeof-at-compile-time/ */
#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)])) // Interesting stuff to read if you are interested to know how this works


#endif
