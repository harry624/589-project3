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

 int new_routing_conn(int sock_index){
     printf("new_routing_conn: %d\n", sock_index);
      int fdaccept, sin_size;
      struct sockaddr_storage remoteaddr;  //conntecter's address information;

      sin_size = sizeof remoteaddr;
      fdaccept = accept(sock_index, (struct sockaddr *)&remoteaddr, &sin_size);
      if(fdaccept == -1){
           perror("accept");
      }

     /* Insert into list of active control connections */
     // connection = malloc(sizeof(struct ControlConn));
     // connection->sockfd = fdaccept;
     // LIST_INSERT_HEAD(&control_conn_list, connection, next);

     return fdaccept;
 }

 void recv_update_distanceVector(int sockfd) {
   char *routing_header, *routing_payload;
   uint16_t num_fields;
   uint32_t sourceIPtmp;
   char sourceIp[40];
   int sourceRouterID;
   /* Get control header */
   routing_header = (char *) malloc(sizeof(char)*ROUTING_HEADER_SIZE);
   bzero(routing_header, ROUTING_HEADER_SIZE);

   //receive header
   if(recvALL(sockfd, routing_header, ROUTING_HEADER_SIZE) < 0){
       // remove_control_conn(sockfd);
       free(routing_header);
       return;
   }

   /* Get control code and payload length from the header */
   #ifdef PACKET_USING_STRUCT
       /** ASSERT(sizeof(struct CONTROL_HEADER) == 8)
         * This is not really necessary with the __packed__ directive supplied during declaration (see control_header_lib.h).
         * If this fails, comment #define PACKET_USING_STRUCT in control_header_lib.h
         */

       BUILD_BUG_ON(sizeof(struct ROUTING_UPDATE_HEADER) != ROUTING_HEADER_SIZE); // This will FAIL during compilation itself; See comment above.
       struct ROUTING_UPDATE_HEADER *header = (struct ROUTING_UPDATE_HEADER *) routing_header;
       num_fields = header->num_fields;
       sourceIPtmp = ntohl(header->sourceIP);

       sprintf(sourceIp, "%d.%d.%d.%d", ((sourceIPtmp>>24)&((1<<8)-1)), ((sourceIPtmp>>16)&((1<<8)-1)), ((sourceIPtmp>>8)&((1<<8)-1)), (sourceIPtmp&((1<<8)-1)));

   #endif
      free(routing_header);

       for (int i = 0; i < 5; i++){
          if (strcmp(sourceIp, routers[i].ipAddress) ){
              sourceRouterID = i + 1;
           }
       }

      //receive routers_cost
      if (num_fields != 0){
          int router_info_payload = sizeof(struct ROUTING_UPDATE_ROUTER) * num_fields;
          routing_payload = (char *) malloc(sizeof(char)*router_info_payload);

          int res = recvALL(sockfd, routing_payload, router_info_payload);
          if(res < 0){
              // remove_control_conn(sock_index);
              free(routing_payload);
              return;
          }

      }

      //update cost
      for (int i = 0; i < num_fields; i++){
          struct ROUTING_UPDATE_ROUTER *router = (struct ROUTING_UPDATE_ROUTER *) (routing_payload + i * 0x0c);


      }
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
