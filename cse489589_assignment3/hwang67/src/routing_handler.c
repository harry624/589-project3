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
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

 #include "../include/global.h"
 #include "../include/author.h"
 #include "../include/control_header_lib.h"
 #include "../include/control_response.h"
 #include "../include/network_util.h"
 #include "../include/routing_handler.h"

 //create UDP socket
 int create_boardcast_UDP_socket(int router_port){
   int r_sock;
   struct addrinfo hints, *servinfo, *p;
   int rv;

   memset(&hints, 0, sizeof hints);
   hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
   hints.ai_socktype = SOCK_DGRAM;
   hints.ai_flags = AI_PASSIVE; // use my IP

   char port[10];
   sprintf(port, "%d", router_port);

   if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
       fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
       return 1;
   }

   // loop through all the results and bind to the first we can
   for(p = servinfo; p != NULL; p = p->ai_next) {
       if ((r_sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
           perror("listener: socket"); continue;
       }
       if (bind(r_sock, p->ai_addr, p->ai_addrlen) == -1) {
           close(r_sock);
           perror("listener: bind");
           continue;
       }
       break;
   }

   if (p == NULL) {
       fprintf(stderr, "listener: failed to bind socket\n");
       return 2;
   }

   freeaddrinfo(servinfo);

   printf("listener: waiting to recvfrom... port: %d\n", router_port);

   return r_sock;
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
        sprintf(sourceIp, "%d.%d.%d.%d", ((tmpIP>>24)&((1<<8)-1)), ((tmpIP>>16)&((1<<8)-1)), ((tmpIP>>8)&((1<<8)-1)), (tmpIP&((1<<8)-1)));

        for (int i = 0; i < 5; i++){
           if (!strcmp(sourceIp, routers[i].ipAddress) ){
               sourceRouterID = i + 1;
            }
        }
        printf("num_fields: %d, sourceRouter_port: %d, sourceIP: %s, id: %d\n", num_fields, source_router_port, sourceIp, sourceRouterID);


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
      char *header_buffer;
      char *router_buffer;
      char *buffer;

      uint16_t num_fields, sourceRouterPort;
      uint32_t sourceIP;

      for (int i = 0; i < 5; i++){
          if (neighbors[i] == 1){
              num_fields++;
          }
      }
      sourceRouterPort = routers[localRouterID - 1].routerPort;
      //cast dot notation to uint32_t
      inet_pton(AF_INET, routers[localRouterID - 1].ipAddress, &sourceIP);

      //header
      uint16_t header_len;
      struct ROUTING_UPDATE_HEADER *header;

      header_buffer = (char *) malloc(sizeof(struct ROUTING_UPDATE_HEADER));
      header = (struct ROUTING_UPDATE_HEADER *) header_buffer;
      /* num_fields */
      header->num_fields = htons(num_fields);
      /* sourceRouterPort */
      header->sourceRouterPort = htons(sourceRouterPort);
      /* sourceIP */
      header->sourceIP = htonl(sourceIP);

      header_len = sizeof(struct ROUTING_UPDATE_HEADER);


      //router_buffer
      uint16_t router_info_len;
      router_buffer = (char *) malloc(sizeof(struct ROUTING_UPDATE_ROUTER) * num_fields);
      int count = 0;
      for (int i = 0; i < 5 && neighbors[i] == 1; i++){
          struct ROUTING_UPDATE_ROUTER *routers_info;
          routers_info = (struct ROUTING_UPDATE_ROUTER *) (router_buffer + count * (sizeof(struct ROUTING_UPDATE_ROUTER)) );
          //cast dot notation to uint32_t
          uint32_t tmpIP;
          inet_pton(AF_INET, routers[i].ipAddress, &tmpIP);
          routers_info->routerIP = tmpIP;
          routers_info->port = routers[i].routerPort;
          routers_info->padding = 0;
          routers_info->routerID = routers[i].routerID;
          routers_info->cost = routers[i].cost;

          count++;
      }

      router_info_len = sizeof(struct ROUTING_UPDATE_ROUTER) * num_fields;
      //merge two buffer
      strcpy(buffer, header_buffer);
      strcat(buffer, router_buffer);

      uint16_t total_len = header_len + router_info_len;

      //boardcast
      sendALL(sockfd, buffer, total_len);

 }
