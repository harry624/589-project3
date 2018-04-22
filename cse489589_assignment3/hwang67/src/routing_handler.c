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

 void update_routing(int sockfd, int neighbors[], struct Router routers[]) {
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
      inet_pton(AF_INET, routers[localRouterID - 1].ipAddress, sourceIP);

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
          inet_pton(AF_INET, routers[i].ipAddress, routers_info->router_IP);
          routers_info->port = routers[i].routerPort;
          // routers_info->padding =
          routers_info->nextHopID = routers[i].nextHopID;
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
