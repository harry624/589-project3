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

#include "../include/control_response.h"
#include "../include/control_header_lib.h"
#include "../include/network_util.h"


void init_response(int sock_index){

  //start broadcasting the routing updates
  //and performing other operations required by the distance vector protocol

}

void routing_table_response(int sock_index, char* cntrl_payload){

}

void update_response(int sock_index, char* cntrl_payload){

}
void crash_response(int sock_index){
    uint16_t payload_len, response_len;
    char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;

    payload_len = 0; // Discount the NULL chararcter
    cntrl_response_payload = (char *) malloc(payload_len);
    memcpy(cntrl_response_payload, "", payload_len);

    cntrl_response_header = create_response_header(sock_index, 4, 4, payload_len);

    response_len = CNTRL_RESP_HEADER_SIZE+payload_len;

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

void sendfile_response(int sock_index, char* cntrl_payload){

}

void sendfile_stats_response(int sock_index, char* cntrl_payload){

}

void last_data_packet_response(int sock_index, char* cntrl_payload){

}
void penultimate_data_packet_response(int sock_index, char* cntrl_payload){

}
