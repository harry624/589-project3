/**
 * @author
 * @author  hao wang <hwang67@buffalo.edu> yue wan <ywan3@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * handle the rest response
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>

#include "../include/global.h"
#include "../include/control_handler.h"
#include "../include/control_response.h"
#include "../include/control_header_lib.h"
#include "../include/network_util.h"
#include "../include/data_handler.h"


void response(int sock_index, int cntr_code, int res_code) {
    uint16_t payload_len, response_len;
    char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;

    payload_len = 0; // Discount the NULL chararcter
    cntrl_response_payload = (char *) malloc(payload_len);
    memcpy(cntrl_response_payload, NULL, payload_len);

    cntrl_response_header = create_response_header(sock_index, cntr_code, res_code, payload_len);

    response_len = CNTRL_RESP_HEADER_SIZE + payload_len;
    cntrl_response = (char *) malloc(response_len);

    /* Copy Header */
    memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
    free(cntrl_response_header);
    /* Copy Payload */
    memcpy(cntrl_response+CNTRL_RESP_HEADER_SIZE, cntrl_response_payload, payload_len);
    free(cntrl_response_payload);

    sendALL(sock_index, cntrl_response, response_len);

    free(cntrl_response);
}

//0x01
void init_response(int sock_index){
    printf("init_response\n");
    response(sock_index, 1, 0);
}

//0x02
void routing_table_response(int sock_index, struct Router routers[num_neighbors]){
    printf("routing_table_response\n");
    //header
    uint16_t payload_len, response_len;
    uint16_t router_info_len = sizeof(struct ROUTER_TABLE_RESPONSE);

    char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;

    payload_len = router_info_len * num_neighbors;

    cntrl_response_header = create_response_header(sock_index, 2, 0, payload_len);

    //response payload
    cntrl_response_payload = (char *) malloc(payload_len);

    for (int i = 0; i < num_neighbors; i++){
        struct ROUTER_TABLE_RESPONSE *router_info;
        router_info = (struct ROUTER_TABLE_RESPONSE *) (cntrl_response_payload + i * router_info_len);
        // printf("allocate memory\n");
        router_info->routerID = htons(routers[i].routerID);
        // printf("id: %d ", router_info->routerID);
        router_info->padding = htons(0);
        // printf("padding: %d ", router_info->padding);
        router_info->nextHopID = htons(routers[i].nextHopID);
        // printf("nexthop: %d ", router_info->nextHopID);
        router_info->cost = htons(distanceVector[localRouterIndex][i]);
        // printf("cost: %d ", router_info->cost);

        printf("routerID:0x%08X, padding: 0x%08X, next_hop: 0x%08X, cost: 0x%08X\n",router_info->routerID, router_info->padding, router_info->nextHopID, router_info->cost);
    }

    response_len = CNTRL_RESP_HEADER_SIZE + payload_len;

    cntrl_response = (char *) malloc(response_len);
    /* Copy Header */
    memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
    free(cntrl_response_header);
    /* Copy Payload */
    memcpy(cntrl_response + CNTRL_RESP_HEADER_SIZE, cntrl_response_payload, payload_len);
    free(cntrl_response_payload);

    sendALL(sock_index, cntrl_response, response_len);

    free(cntrl_response);

}

//0x03
void update_response(int sock_index){
    response(sock_index, 3, 0);
}

//0x04
void crash_response(int sock_index){
    response(sock_index, 4, 0);
}

//0x05
void sendfile_response(int sock_index){
    printf("sendfile_response\n");
    response(sock_index, 5, 0);
}

//0x06
void sendfile_stats_response(int sock_index, char* cntrl_payload){
    uint8_t transferID;
    uint16_t padding = htons(0);
    uint16_t varSeqNum;
    //get transferID
    memcpy(&transferID, cntrl_payload, sizeof(transferID));

    char *response_header;
    uint16_t response_len;
    char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;

    if (fileStatArray[transferID].index == 0){
        response(sock_index, 6, 0);
        return;
    }

    int file_stat_index = fileStatArray[transferID].index;
    int payload_len = 0x04 + (file_stat_index) * sizeof (uint16_t);

    cntrl_response_header = create_response_header(sock_index, 6, 0, payload_len);

    cntrl_response_payload = (char*)malloc(sizeof(char)*payload_len);
    memcpy(cntrl_response_payload, &transferID, sizeof(transferID));
    memcpy(cntrl_response_payload + 0x01, &fileStatArray[transferID].ttl, sizeof(fileStatArray[transferID].ttl));
    memcpy(cntrl_response_payload + 0x02, &padding, sizeof(padding));
    int offset = 0x04;

    for(int i=0; i < file_stat_index; i++){
        memcpy(cntrl_response_payload + offset, &fileStatArray[transferID].seq_num_array[i], sizeof(fileStatArray[transferID].seq_num_array[i]));
        offset = offset + sizeof(fileStatArray[transferID].seq_num_array[i]);
    }

    cntrl_response = (char*)malloc(CNTRL_RESP_HEADER_SIZE + payload_len);

    //copy header
    memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
    free(cntrl_response_header);

    //copy payload
    memcpy(cntrl_response + CNTRL_RESP_HEADER_SIZE, cntrl_response_payload, payload_len);
    free(cntrl_response_payload);

    sendALL(sock_index, cntrl_response, CNTRL_RESP_HEADER_SIZE + payload_len);

    free(cntrl_response);
    return;
}

//0x07
void last_data_packet_response(int sock_index){
    if (lastDataPacket[0] == '\0'){
        response(sock_index, 7, 0);
        return;
    }
    char *cntrl_response_header, *cntrl_response;

    cntrl_response_header = create_response_header(sock_index, 7, 0, DATA_PACKET_SIZE);
    cntrl_response = (char*)malloc(sizeof(char) * (CNTRL_RESP_HEADER_SIZE + DATA_PACKET_SIZE));

    //copy header
    memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
    free(cntrl_response_header);

    //copy packet
    memcpy(cntrl_response + CNTRL_RESP_HEADER_SIZE, lastDataPacket, DATA_PACKET_SIZE);
    sendALL(sock_index, cntrl_response, CNTRL_RESP_HEADER_SIZE + DATA_PACKET_SIZE);
    free(cntrl_response);

    return;
}

//0x08
void penultimate_data_packet_response(int sock_index){
    if(penultimateDataPacket[0] == '\0'){
        response(sock_index, 8, 0);
        return;
    }
    char *cntrl_response_header, *cntrl_response;

    cntrl_response_header = create_response_header(sock_index, 8, 0, DATA_PACKET_SIZE);
    cntrl_response = (char*)malloc(sizeof(char) * (CNTRL_RESP_HEADER_SIZE + DATA_PACKET_SIZE));

    //copy header
    memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
    free(cntrl_response_header);

    //copy packet
    memcpy(cntrl_response + CNTRL_RESP_HEADER_SIZE, penultimateDataPacket, DATA_PACKET_SIZE);
    // free(cntrl_response_header);

    sendALL(sock_index, cntrl_response, CNTRL_RESP_HEADER_SIZE + DATA_PACKET_SIZE);
    free(cntrl_response);

    return;
}
