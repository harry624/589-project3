#ifndef DATA_HANDLER_H_
#define DATA_HANDLER_H_

#define DATA_PACKET_HEADER_SIZE 12

struct __attribute__((__packed__)) DATA_PACKET_HEADER
{
    uint32_t destationIP;
    uint8_t transferID;
    uint8_t TTL;
    uint16_t seq_num;
    uint16_t FIN;
    uint16_t padding;
};


struct __attribute__((__packed__)) SEND_FILE_CONTROL
{
    uint32_t destationIP;
    uint8_t init_TTL;
    uint8_t transferID;
    uint16_t init_seq_num;
};

struct __attribute__((__packed__)) SEND_FILE_PACKET
{
    uint32_t dest_ip;
    uint8_t transfer_id;
    uint8_t ttl;
    uint16_t seq_no;
    uint16_t FIN;
    uint16_t padding;
    char *payload;
};

void create_data_socket(uint16_t dataPort);

int new_data_conn(int sock_index);

int isData(int sock_index);

int data_recv_hook(int sock_index);

void send_file(int sock_index, char * cntrl_payload, uint16_t payload_len);

#endif
