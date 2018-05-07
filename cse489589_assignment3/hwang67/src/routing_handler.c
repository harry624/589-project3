/**
 * @network_util
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
 * handle routing update packets
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

 #include "../include/global.h"
 #include "../include/author.h"
 #include "../include/connection_manager.h"
 #include "../include/control_handler.h"
 #include "../include/control_header_lib.h"
 #include "../include/control_response.h"
 #include "../include/network_util.h"
 #include "../include/routing_handler.h"

//create router_recv_socket
void create_router_socket(uint16_t routerPort) {
    printf("create_router_socket\n");

     // create UDP socket
     router_socket = create_UDP_listener_socket(routerPort);

     FD_SET(router_socket, &master);

     if(router_socket > fdmax) fdmax = router_socket;

     printf("router_socket: %d, fdmax: %d, is in the list: %d\n", router_socket, fdmax, FD_ISSET(router_socket, &master));

    return;
}


 //create UDP socket
int create_UDP_listener_socket(uint16_t router_port){
     struct addrinfo hints, *res;
     int sockfd;
     struct sockaddr_in router_addr;
     socklen_t addr_len;

     memset(&hints, 0, sizeof hints);
     hints.ai_family = AF_UNSPEC;
     hints.ai_socktype = SOCK_DGRAM;

     // make a socket;
     if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
         perror("fail to create socket");
     }
     /* Make socket re-usable */
     if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) < 0){
         perror("setsockopt");
         exit(1);
     }

     if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (int[]){1}, sizeof(int)) < 0){
         perror("setsockopt");
         exit(1);
     }

     bzero(&router_addr, sizeof(router_addr));

     router_addr.sin_family = AF_INET;
     router_addr.sin_addr.s_addr = htonl(INADDR_ANY);
     router_addr.sin_port = htons(router_port);

     if(bind(sockfd, (struct sockaddr *)&router_addr, sizeof(router_addr)) < 0){
         perror("server: bind");
         exit(1);
     }

     printf("listener: waiting to recvfrom... port: %d\n", router_port);

     return sockfd;
}

//receive UDP broadcast
void updateDVBybellmanFord() {
    // for (int i = 0; i < num_neighbors; i++){
    //     for (int j = 0; j < num_neighbors; j++){
    //         if (distanceVector[localRouterIndex][i] > distanceVector[localRouterIndex][j] + distanceVector[j][i]){
    //             distanceVector[localRouterIndex][i] = distanceVector[localRouterIndex][j] + distanceVector[j][i];
    //             routers[i].nextHopID = routers[j].routerID;
    //             routers[i].cost = distanceVector[localRouterIndex][i];
    //             printf("update index:%d, nextHopID is: %d\n", i, routers[i].nextHopID);
    //         }
    //     }
    // }

    int count = 0;
    do {
        int count = 0;
        for (int i = 0; i < num_neighbors; i++){
            for (int j = 0; j < num_neighbors; j++){
                for (int k = 1; k < num_neighbors; k++){
                    if (distanceVector[i][j] > distanceVector[i][k] + distanceVector[k][j]){
                        distanceVector[i][j] = distanceVector[i][k] + distanceVector[k][j];
                        count++;
                        if (i == localRouterIndex){
                            routers[j].nextHopID = routers[k].routerID;
                            routers[j].cost = distanceVector[i][j];
                        }
                    }
                    if (distanceVector[i][j] > INF){
                        distanceVector[i][j] = INF;
                    }
                }
            }
        }
    }while(count != 0);

    // for (int i = 0; i < num_neighbors; i++){
    //     for (int j = 0; j < num_neighbors; j++){
    //         if(i == localRouterIndex){
    //           printf("%d, next hopid: %d\t", distanceVector[i][j], routers[j].nextHopID);
    //
    //         }else{
    //           printf("%d\t", distanceVector[i][j]);
    //         }
    //     }
    //     printf("\n");
    // }

    return;
}

int recv_update_distanceVector(int sockfd) {
        // printf("recv_update_distanceVector\n");
        char *routing_header, *routing_payload;
        char *routing_update;
        uint16_t num_fields;

        char sourceIp[40];
        int sourceRouterID;
        int sourceRouterIndex;
        /* Get control header */
        routing_header = (char *) malloc(sizeof(char)*ROUTING_HEADER_SIZE);
        bzero(routing_header, ROUTING_HEADER_SIZE);

        int router_info_payload = sizeof(struct ROUTING_UPDATE_ROUTER) * num_neighbors;

        routing_payload = (char *) malloc(sizeof(char)*router_info_payload);

        ssize_t nbytes = ROUTING_HEADER_SIZE + router_info_payload;
        routing_update = (char *) malloc(nbytes);
        bzero(routing_update, nbytes);

        // int res = recvfromALL(sockfd, routing_update, nbytes);

        struct sockaddr_storage from;
        socklen_t fromlen;
        fromlen = sizeof from;
        ssize_t res = 0;
        res = recvfrom(sockfd, routing_update, nbytes, 0, (struct sockaddr *)&from, &fromlen);

        if (res < 0){
           perror("receive broadcast failed");
           return -1;
        }
        // printf("received :%d\n", res);

        if (res < 8){
          return -1;
        }

        /* Get control code and payload length from the header */
        struct ROUTING_UPDATE_HEADER *header = (struct ROUTING_UPDATE_HEADER *) routing_update;
        num_fields = ntohs(header->num_fields);
        uint16_t source_router_port = ntohs(header->sourceRouterPort);
        uint32_t tmpIP = ntohl(header->sourceIP);
        sprintf(sourceIp, "%d.%d.%d.%d",
                ((tmpIP>>24)&((1<<8)-1)), ((tmpIP>>16)&((1<<8)-1)), ((tmpIP>>8)&((1<<8)-1)), (tmpIP&((1<<8)-1)));

        //find the source boardcast router index
        for (int i = 0; i < num_neighbors; i++){
             if (tmpIP == routers[i].int32_ip){
                 // sourceRouterID = i + 1;
                 sourceRouterIndex = i;
                 routers[i].missedcnt = 0;
                 routers[i].firstupdateReceived = 1;
                 break;
              }
        }
        //
        // printf("num_fields: %d, sourceRouter_port: %d, sourceIP: %s, r_table_index: %d\n",
        //           num_fields, source_router_port, sourceIp, sourceRouterIndex);

        //udpate routing table row of source router ID
        int routerIndex = 0;
        for (int i = 0; i < num_fields; i++){
            struct ROUTING_UPDATE_ROUTER *router_update = (struct ROUTING_UPDATE_ROUTER *) (routing_update + ROUTING_HEADER_SIZE + i * 0x0c);
            uint16_t routerId = ntohs(router_update->routerID);
            uint16_t cost = ntohs(router_update->cost);

            //get routerID index in router array
            for (int j = 0; j < num_neighbors; j++){
                if (routers[j].routerID == routerId){
                    routerIndex = j;
                    routers[j].cost = cost;
                    break;
                }
            }

            //check if the cost have been increase and update the cost accordingly before run the bellmanford alg
            // printf("routerID: %d, old cost: %d, new cost: %d\n", routerId,distanceVector[sourceRouterIndex][routerIndex], cost);
            if (distanceVector[sourceRouterIndex][routerIndex] < cost && routers[routerIndex].nextHopID == routers[sourceRouterIndex].routerID){
                // distanceVector[localRouterIndex][routerIndex] = cost + distanceVector[sourceRouterIndex][localRouterIndex];
                distanceVector[localRouterIndex][routerIndex] = INF;
                routers[routerIndex].nextHopID = INF;

                // if (distanceVector[localRouterIndex][routerIndex]  > INF){
                //     distanceVector[localRouterIndex][routerIndex] = INF;
                //     routers[routerIndex].nextHopID = INF;
                // }
            }

            distanceVector[sourceRouterIndex][routerIndex] = cost;
        }

        // for (int i = 0; i < num_neighbors; i++){
        //     if (i == localRouterIndex || neighbors[i] == 1){
        //         continue;
        //     }
        //
        //     // printf("update routing table index: %d, not neighbors\n", i);
        //     for (int j = 0; j < num_neighbors; j++){
        //         if (distanceVector[j][i] < INF){
        //             distanceVector[i][j] = distanceVector[j][i];
        //         }
        //     }
        // }

        updateDVBybellmanFord();

        return sourceRouterIndex;
}

void boardcast_update_routing(int sockfd, int neighbors[], struct Router routers[]) {
       // printf("boardcast_update_routing\n");
       char *update_header, *update_payload, *router_update;

       uint16_t num_fields, sourceRouterPort;
       uint32_t sourceIP;

       num_fields = num_neighbors;

       sourceRouterPort = routers[localRouterIndex].routerPort;
       //cast dot notation to uint32_t
       inet_pton(AF_INET, routers[localRouterIndex].ipAddress, &sourceIP);

       //header
       uint16_t header_len;
       struct ROUTING_UPDATE_HEADER *header;

       update_header = (char *) malloc(sizeof(struct ROUTING_UPDATE_HEADER));
       header = (struct ROUTING_UPDATE_HEADER *) update_header;
       /* num_fields */
       header->num_fields = htons(num_fields);
       /* sourceRouterPort */
       header->sourceRouterPort = htons(sourceRouterPort);
       /* sourceIP */
       header->sourceIP = sourceIP;

       header_len = sizeof(struct ROUTING_UPDATE_HEADER);

       // printf("header created\n");
       //router_buffer
       uint16_t update_payload_len;
       update_payload = (char *) malloc(sizeof(struct ROUTING_UPDATE_ROUTER) * num_fields);

       for (int i = 0; i < num_fields && routers[i].isRemoved == 0; i++){
           struct ROUTING_UPDATE_ROUTER *routers_info;
           routers_info = (struct ROUTING_UPDATE_ROUTER *) (update_payload + i * (sizeof(struct ROUTING_UPDATE_ROUTER)) );
           //cast dot notation to uint32_t
           uint32_t tmpIP;
           inet_pton(AF_INET, routers[i].ipAddress, &tmpIP);
           routers_info->routerIP = htonl(tmpIP);
           routers_info->port = htons(routers[i].routerPort);
           routers_info->padding = htons(0);
           routers_info->routerID = htons(routers[i].routerID);
           routers_info->cost = htons(distanceVector[localRouterIndex][i]);
       }

       // printf("payload created\n");

       update_payload_len = sizeof(struct ROUTING_UPDATE_ROUTER) * num_fields;

       //merge header and payload
       uint16_t total_len = header_len + update_payload_len;

       router_update = (char *) malloc(total_len);
       /* Copy Header */
       memcpy(router_update, update_header, ROUTING_HEADER_SIZE);
       free(update_header);
       /* Copy Payload */
       memcpy(router_update + ROUTING_HEADER_SIZE, update_payload, update_payload_len);
       free(update_payload);

       // printf("merged\n");

       //boardcast
       struct sockaddr_in to;
       int addr_len = sizeof(to);
       for (int i = 0; i < num_neighbors; i++){
             if (neighbors[i] == 1 && routers[i].isRemoved == 0){
                 bzero (&to, sizeof(to));
                 to.sin_family = AF_INET;
                 inet_pton(AF_INET, routers[i].ipAddress, &to.sin_addr);
                 to.sin_port   = htons(routers[i].routerPort);

                 int res = sendtoALL(sockfd, router_update, total_len, to);

                 if (res < 0){
                   perror("send UDP broadcast error");
                   return;
                 }
                 // printf("send to neighbors: %d, destip: %s, router port: %d, sent:%d\n",
                 //          i+1, routers[i].ipAddress, routers[i].routerPort, res);
             }
       }

       close(sockfd);
       return;
}
