#ifndef CONTROL_RESPONSE_H_
#define CONTROL_RESPONSE_H_

#define PACKET_USING_STRUCT // Comment this out to use alternate packet crafting technique

#ifdef PACKET_USING_STRUCT
    struct __attribute__((__packed__)) ROUTER_INIT
    {

        uint16_t routerID;
        uint16_t port_1;
        uint16_t port_2;
        uint16_t cost;
        uint32_t ipAddress;
    };

    struct __attribute__((__packed__)) ROUTER_INIT_RESPONSE_HEADER
    {
        uint16_t routerID;
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

void routing_table_response(int sock_index, char* cntrl_payload);

void update_response(int sock_index, char* cntrl_payload);

void crash_response(int sock_index);

void sendfile_response(int sock_index, char* cntrl_payload);

void sendfile_stats_response(int sock_index, char* cntrl_payload);

void last_data_packet_response(int sock_index, char* cntrl_payload);

void penultimate_data_packet_response(int sock_index, char* cntrl_payload);


#endif
