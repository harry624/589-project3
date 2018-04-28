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
 #include "../include/control_header_lib.h"
 #include "../include/control_response.h"
 #include "../include/network_util.h"
 #include "../include/routing_handler.h"

 //create UDP socket
int create_boardcast_UDP_socket(int router_port){
   // int r_sock;
   // struct addrinfo hints, *servinfo, *p;
   // int rv;
   //
   // memset(&hints, 0, sizeof hints);
   // hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
   // hints.ai_socktype = SOCK_DGRAM;
   // hints.ai_flags = AI_PASSIVE; // use my IP
   //
   // char port[10];
   // sprintf(port, "%d", router_port);
   //
   // if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
   //     fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
   //     return 1;
   // }
   //
   // // loop through all the results and bind to the first we can
   // for(p = servinfo; p != NULL; p = p->ai_next) {
   //     if ((r_sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
   //         perror("listener: socket"); continue;
   //     }
   //     if (bind(r_sock, p->ai_addr, p->ai_addrlen) == -1) {
   //         close(r_sock);
   //         perror("listener: bind");
   //         continue;
   //     }
   //     break;
   // }
   //
   // if (p == NULL) {
   //     fprintf(stderr, "listener: failed to bind socket\n");
   //     return 2;
   // }
   //
   // freeaddrinfo(servinfo);
   //
   // printf("listener: waiting to recvfrom... port: %d\n", router_port);
   //
   // return r_sock;

     struct sockaddr_in control_addr;
     socklen_t addrlen = sizeof(control_addr);

     int sock = socket(AF_INET, SOCK_DGRAM, 0);
     if(sock < 0)
         perror("socket() failed");

     /* Make socket re-usable */
     // if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) < 0)
   // 	perror("setsockopt() failed");

     bzero(&control_addr, sizeof(control_addr));

     control_addr.sin_family = AF_INET;
     control_addr.sin_addr.s_addr = htonl(INADDR_ANY);
     control_addr.sin_port = htons(router_port);

     if(bind(sock, (struct sockaddr *)&control_addr, sizeof(control_addr)) < 0)
     perror("bind() failed");

     //LIST_INIT(&control_conn_list);
     return sock;
}

void recv_update_distanceVector(int sockfd) {
        printf("recv_update_distanceVector\n");
        char *routing_header, *routing_payload;
        char *routing_update;
        uint16_t num_fields;

        char sourceIp[40];
        int sourceRouterID;
        /* Get control header */
        routing_header = (char *) malloc(sizeof(char)*ROUTING_HEADER_SIZE);
        bzero(routing_header, ROUTING_HEADER_SIZE);

        int router_info_payload = sizeof(struct ROUTING_UPDATE_ROUTER) * 5;

        routing_payload = (char *) malloc(sizeof(char)*router_info_payload);

        ssize_t nbytes = ROUTING_HEADER_SIZE + router_info_payload;
        routing_update = (char *) malloc(nbytes);
        bzero(routing_update, nbytes);

        int res = recvfromALL(sockfd, routing_update, nbytes);

        if (res < 0){
           return;
        }
        printf("received :%d\n", res);

    /* Get control code and payload length from the header */
        struct ROUTING_UPDATE_HEADER *header = (struct ROUTING_UPDATE_HEADER *) routing_update;
        num_fields = ntohs(header->num_fields);
        uint16_t source_router_port = ntohs(header->sourceRouterPort);
        uint32_t tmpIP = ntohl(header->sourceIP);
        sprintf(sourceIp, "%d.%d.%d.%d",
                ((tmpIP>>24)&((1<<8)-1)), ((tmpIP>>16)&((1<<8)-1)), ((tmpIP>>8)&((1<<8)-1)), (tmpIP&((1<<8)-1)));

        for (int i = 0; i < 5; i++){
           if (!strcmp(sourceIp, routers[i].ipAddress) ){
               sourceRouterID = i + 1;
            }
        }
        printf("num_fields: %d, sourceRouter_port: %d, sourceIP: %s, id: %d\n",
                  num_fields, source_router_port, sourceIp, sourceRouterID);

        //udpate routing table
        for (int i = 0; i < num_fields; i++){
            struct ROUTING_UPDATE_ROUTER *router_update = (struct ROUTING_UPDATE_ROUTER *) (routing_update + ROUTING_HEADER_SIZE + i * 0x0c);
            uint16_t routerId = ntohs(router_update->routerID);
            uint16_t cost = ntohs(router_update->cost);
            if(routers[routerId].nextHopID == INF){
                routers[routerId].nextHopID = sourceRouterID;
                printf("get update router id :%d\n", routerId);

                uint16_t newCost = routers[sourceRouterID].cost + cost;
                if (routers[routerId].cost > newCost){
                    routers[routerId].cost = newCost;
                    distanceVector[localRouterID-1][routerId-1] = newCost;
                }
            }
        }
        // boardcast_update_routing(sockfd, neighbors, routers);
        return;
}

void boardcast_update_routing(int sockfd, int neighbors[], struct Router routers[]) {
       printf("boardcast_update_routing\n");
       char *update_header, *update_payload, *router_update;

       uint16_t num_fields, sourceRouterPort;
       uint32_t sourceIP;

       num_fields = num_neighbors;

       sourceRouterPort = routers[localRouterID - 1].routerPort;
       //cast dot notation to uint32_t
       inet_pton(AF_INET, routers[localRouterID - 1].ipAddress, &sourceIP);

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

       for (int i = 0; i < num_fields; i++){
           struct ROUTING_UPDATE_ROUTER *routers_info;
           routers_info = (struct ROUTING_UPDATE_ROUTER *) (update_payload + i * (sizeof(struct ROUTING_UPDATE_ROUTER)) );
           //cast dot notation to uint32_t
           uint32_t tmpIP;
           inet_pton(AF_INET, routers[i].ipAddress, &tmpIP);
           routers_info->routerIP = htonl(tmpIP);
           routers_info->port = htons(routers[i].routerPort);
           routers_info->padding = htons(0);
           routers_info->routerID = htons(routers[i].routerID);
           routers_info->cost = htons(routers[i].cost);

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
       for (int i = 0; i < 5; i++){
             if (neighbors[i] == 1){
                 bzero (&to, sizeof(to));
                 to.sin_family = AF_INET;
                 inet_pton(AF_INET, routers[i].ipAddress, &to.sin_addr);
                 to.sin_port   = htons(routers[i].routerPort);

                 int res = sendtoALL(sockfd, router_update, total_len, to);
                 if (res < 0){
                   return;
                 }
                 printf("send to neighbors: %d, destip: %s, router port: %d, sent:%d\n",
                          i+1, routers[i].ipAddress, routers[i].routerPort, res);
             }
       }

       close(sockfd);
       return;
}
