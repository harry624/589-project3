/**
 * @ubitname_assignment3
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
 * Handler for the control plane.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/queue.h>
#include <unistd.h>

#include "../include/author.h"
#include "../include/global.h"
#include "../include/control_handler.h"
#include "../include/connection_manager.h"
#include "../include/control_header_lib.h"
#include "../include/control_response.h"
#include "../include/routing_handler.h"
#include "../include/network_util.h"

#ifndef PACKET_USING_STRUCT
    #define CNTRL_CONTROL_CODE_OFFSET 0x04
    #define CNTRL_PAYLOAD_LEN_OFFSET 0x06
#endif

int distanceVector[5][5];
int neighbors[5];
/* Linked List for active control connections */
struct ControlConn
{
    int sockfd;
    LIST_ENTRY(ControlConn) next;
}*connection, *conn_temp;


LIST_HEAD(ControlConnsHead, ControlConn) control_conn_list;

//create UDP socket
int create_boardcast_UDP(int router_port){
  struct addrinfo hints, *res;
  int router_socket;
  struct sockaddr_in router_addr;
  socklen_t addr_len;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;

  // char portchar[10];
  // sprintf(portchar, "%d", router_port);

  //make a socket;
  if ((router_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
      perror("fail to create socket");
  }

  /* Make socket re-usable */
  if(setsockopt(router_socket, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) < 0){
      perror("setsockopt");
      exit(1);
  }
  bzero(&router_addr, sizeof(router_addr));

  router_addr.sin_family = AF_INET;
  router_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  router_addr.sin_port = router_port;

  if(bind(router_socket, (struct sockaddr *)&router_addr, sizeof(router_addr)) < 0){
      perror("server: bind");
      exit(1);
  }

  // if(listen(router_socket, BACKLOG) < 0){
  //     perror("listen");
  //     exit(1);
  // }


  return router_socket;
}

//create socket
int create_control_socket(){
    int sock;
    struct sockaddr_in control_addr;
    socklen_t addrlen = sizeof(control_addr);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0){
      perror("server: socket");

    }
    /* Make socket re-usable */
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) < 0){
        perror("setsockopt");
        exit(1);
    }

    bzero(&control_addr, sizeof(control_addr));

    control_addr.sin_family = AF_INET;
    control_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    control_addr.sin_port = htons(CONTROL_PORT);

    if(bind(sock, (struct sockaddr *)&control_addr, sizeof(control_addr)) < 0){
        perror("server: bind");
        exit(1);
    }

    if(listen(sock, BACKLOG) < 0){
        perror("listen");
        exit(1);
    }

    LIST_INIT(&control_conn_list);

    return sock;
}


void init_table(char *cntrl_payload) {

    uint16_t num_neighbors, update_interval;
    /* Get control code and payload length from the header */
    memcpy(&num_neighbors, cntrl_payload, sizeof(num_neighbors));
    memcpy(&update_interval, cntrl_payload + 0x02, sizeof(update_interval));

    num_neighbors = ntohs(num_neighbors);
    update_interval = ntohs(update_interval);

    printf("number of neighbors: %d, size: %d, update_interval: %d, size: %d\n", num_neighbors, sizeof(num_neighbors),update_interval, sizeof(update_interval));

    //init neighbors array
    for (int i = 0; i < num_neighbors - 1; i++){
        neighbors[i] = 0;
    }

    #ifdef PACKET_USING_STRUCT
        // BUILD_BUG_ON(sizeof(struct ROUTER_INIT) != 512); // This will FAIL during compilation itself; See comment above.

        //save all the routers info in an array
        for (int i = 0; i < num_neighbors; i++){
            struct ROUTER_INIT *init = (struct ROUTER_INIT *) (cntrl_payload + 0x04 + i * 0x0c);
            routers[i].routerID = ntohs(init->routerID);
            routers[i].routerPort = ntohs(init->routerPort);
            routers[i].dataPort = ntohs(init->dataPort);
            routers[i].cost = ntohs(init->cost);
            uint32_t tmpIP = ntohl(init->ipAddress);
            sprintf(routers[i].ipAddress, "%d.%d.%d.%d", ((tmpIP>>24)&((1<<8)-1)), ((tmpIP>>16)&((1<<8)-1)), ((tmpIP>>8)&((1<<8)-1)), (tmpIP&((1<<8)-1)));

            printf("routerID:%d, port_1: %d, port_2: %d, cost: %d, ipAddress: %s\n",routers[i].routerID, routers[i].routerPort, routers[i].dataPort, routers[i].cost, routers[i].ipAddress);

            routers[i].missedCnt = 0;
            routers[i].nextHopID = ntohs(init->routerID);

            if (routers[i].cost == INF){
                routers[i].nextHopID = INF;
            }else if (routers[i].cost == 0){
                localRouterID = routers[i].routerID;

                // routers[i].UDPsockfd = router_socket;
                //reset timer

            }else{
                //update neighbors array
                neighbors[i] = 1;
                //boardcast the routing updates
                // routers[i].UDPsockfd =
                //  = routers[i].UDPsockfd;
            }
        }

    #endif



    //create table
    printf("distance vector:\n");
    for (int i = 0; i < num_neighbors; i++){
        for (int j = 0; j < num_neighbors; j++){
            if (i == localRouterID-1){
                distanceVector[i][j] = routers[j].cost;
            }else{
                distanceVector[i][j] = INF;
            }
            printf("%d ", distanceVector[i][j]);
        }
        printf("\n");
    }

    // //boardcast the routing updates
    // for (int i = 0; i < num_neighbors && neighbors[i] == 1; i++){
    //     boardcast_update_routing(routers[i].UDPsockfd, neighbors, routers);
    // }
}

//update router cost
void updateCost(char *cntrl_payload){
    uint16_t routerID, cost;
    /* Get control code and payload length from the header */
    #ifdef PACKET_USING_STRUCT

        // BUILD_BUG_ON(sizeof(struct CONTROL_HEADER) != CNTRL_HEADER_SIZE); // This will FAIL during compilation itself; See comment above.
        struct COST_UPDATE *cost_update = (struct COST_UPDATE *) cntrl_payload;
        routerID = ntohs(cost_update->routerID);
        cost = ntohs(cost_update->cost);
    #endif

    //update local table
    distanceVector[localRouterID-1][routerID-1] = cost;

    //boardcast

}

//0x04 crash
void crash_router(int sock_index){

	close(router_socket);
	FD_CLR(sock_index, &master);
}

//receive file
void receive_file(char *cntrl_payload){

}


void remove_control_conn(int sock_index){
    printf("remove_control_conn:%d\n", sock_index);
    LIST_FOREACH(connection, &control_conn_list, next) {
        if(connection->sockfd == sock_index) LIST_REMOVE(connection, next); // this may be unsafe?
        free(connection);
    }

    close(sock_index);
}

int new_control_conn(int sock_index){
    printf("new_control_conn: %d\n", sock_index);
     int fdaccept, sin_size;
     struct sockaddr_storage remoteaddr;  //conntecter's address information;

     sin_size = sizeof remoteaddr;
     fdaccept = accept(sock_index, (struct sockaddr *)&remoteaddr, &sin_size);
     if(fdaccept == -1){
          perror("accept");
     }

    /* Insert into list of active control connections */
    connection = malloc(sizeof(struct ControlConn));
    connection->sockfd = fdaccept;
    LIST_INSERT_HEAD(&control_conn_list, connection, next);

    return fdaccept;
}

int isControl(int sock_index){
    // printf("is control: %d\n", sock_index);
    LIST_FOREACH(connection, &control_conn_list, next)
        if(connection->sockfd == sock_index) return 1;

    return 0;
}

int control_recv_hook(int sock_index){
    printf("is control_recv_hook: %d\n", sock_index);
    char *cntrl_header, *cntrl_payload;
    uint8_t control_code;
    uint16_t payload_len;

    /* Get control header */
    cntrl_header = (char *) malloc(sizeof(char)*CNTRL_HEADER_SIZE);
    bzero(cntrl_header, CNTRL_HEADER_SIZE);

    if(recvALL(sock_index, cntrl_header, CNTRL_HEADER_SIZE) < 0){
        remove_control_conn(sock_index);
        free(cntrl_header);
        return 0;
    }

    /* Get control code and payload length from the header */
    #ifdef PACKET_USING_STRUCT
        /** ASSERT(sizeof(struct CONTROL_HEADER) == 8)
          * This is not really necessary with the __packed__ directive supplied during declaration (see control_header_lib.h).
          * If this fails, comment #define PACKET_USING_STRUCT in control_header_lib.h
          */

        BUILD_BUG_ON(sizeof(struct CONTROL_HEADER) != CNTRL_HEADER_SIZE); // This will FAIL during compilation itself; See comment above.
        struct CONTROL_HEADER *header = (struct CONTROL_HEADER *) cntrl_header;
        control_code = header->control_code;
        payload_len = ntohs(header->payload_len);
    #endif
    #ifndef PACKET_USING_STRUCT
        memcpy(&control_code, cntrl_header+CNTRL_CONTROL_CODE_OFFSET, sizeof(control_code));
        memcpy(&payload_len, cntrl_header+CNTRL_PAYLOAD_LEN_OFFSET, sizeof(payload_len));
        payload_len = ntohs(payload_len);
    #endif

    free(cntrl_header);

    /* Get control payload */
    if(payload_len != 0){
        printf("payload_len: %d\n", payload_len);
        cntrl_payload = (char *) malloc(sizeof(char)*payload_len);
        bzero(cntrl_payload, payload_len);

        int res = recvALL(sock_index, cntrl_payload, payload_len);
        if(res < 0){
            remove_control_conn(sock_index);
            free(cntrl_payload);
            return 0;
        }
    }

    printf("control_code: %d\n", control_code);

    /* Triage on control_code */
    switch(control_code){

        //AUTHOR [Control Code: 0x00]
        case 0: author_response(sock_index);
                break;

        //INIT [Control Code: 0x01]
        case 1:
                init_table(cntrl_payload);
                init_response(sock_index);
                break;

        //ROUTING-TABLE [Control Code: 0x02]
        case 2: routing_table_response(sock_index, routers);
                break;

        //UPDATE [Control Code: 0x03]
        case 3:
                updateCost(cntrl_payload);
                update_response(sock_index);
                break;

        //CRASH [Control Code: 0x04]
        case 4:
                crash_router(router_socket);
                crash_response(sock_index);
                exit(1);
                break;

        //SENDFILE [Control Code: 0x05]
        case 5:
                receive_file(cntrl_payload);
                sendfile_response(sock_index);
                break;

        //SENDFILE-STATS [Control Code: 0x06]
        case 6: sendfile_stats_response(sock_index, cntrl_payload);
                break;

        //LAST-DATA-PACKET [Control Code: 0x07]
        case 7: last_data_packet_response(sock_index);
                break;

        //PENULTIMATE-DATA-PACKET [Control Code: 0x08]
        case 8: penultimate_data_packet_response(sock_index);
                break;

    }

    if(payload_len != 0) free(cntrl_payload);
    return 1;
}
