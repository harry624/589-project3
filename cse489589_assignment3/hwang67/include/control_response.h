#ifndef CONTROL_RESPONSE_H_
#define CONTROL_RESPONSE_H_

#include "../include/global.h"


#define PACKET_USING_STRUCT // Comment this out to use alternate packet crafting technique

#ifdef PACKET_USING_STRUCT
    struct __attribute__((__packed__)) ROUTER_INIT
    {

        uint16_t routerID;
        uint16_t routerPort;
        uint16_t dataPort;
        uint16_t cost;
        uint32_t ipAddress;
    };

    struct __attribute__((__packed__)) ROUTER_TABLE_RESPONSE_HEADER
    {
        uint16_t routerID;
        uint16_t padding;
        uint16_t nextHopID;
        uint16_t cost;
    };

    struct __attribute__((__packed__)) COST_UPDATE
    {
        uint16_t routerID;
        uint16_t cost;
    };

    struct __attribute__((__packed__)) SEND_FILE
    {
        uint16_t routerID;
        uint16_t cost;
    };

#endif


void init_response(int sock_index);

void routing_table_response(int sock_index, struct Router routers[]);

void update_response(int sock_index);

void crash_response(int sock_index);

void sendfile_response(int sock_index);

void sendfile_stats_response(int sock_index, char* cntrl_payload);

void last_data_packet_response(int sock_index, char* cntrl_payload);

void penultimate_data_packet_response(int sock_index, char* cntrl_payload);


#endif
