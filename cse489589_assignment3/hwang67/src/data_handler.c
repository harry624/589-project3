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
 * Handler for the data plane.
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
#include "../include/data_handler.h"

struct DataConn
{
     int sockfd;
     LIST_ENTRY(DataConn) next;
}*connection, *conn_temp;

 LIST_HEAD(DataConnsHead, DataConn) data_conn_list;


int isData(int sock_index){
	//cout<<"Is Data"<<endl;
	 LIST_FOREACH(connection, &data_conn_list, next)
		 if(connection->sockfd == sock_index) return 1;
	 return 0;
 }

void create_data_socket(uint16_t dataPort) {
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
    control_addr.sin_port = htons(dataPort);

    if(bind(sock, (struct sockaddr *)&control_addr, sizeof(control_addr)) < 0){
        perror("server: bind");
        exit(1);
    }

    if(listen(sock, BACKLOG) < 0){
        perror("listen");
        exit(1);
    }

    LIST_INIT(&data_conn_list);

    data_socket = sock;

    FD_SET(data_socket, &master);

    if(data_socket > fdmax) fdmax = data_socket;

    return;
}

int new_data_conn(int sock_index)
{
     int fdaccept;
     socklen_t caddr_len;
     struct sockaddr_in remote_controller_addr;

     caddr_len = sizeof(remote_controller_addr);
     fdaccept = accept(sock_index, (struct sockaddr *)&remote_controller_addr, &caddr_len);
     if(fdaccept < 0)
		 perror("accept() failed");

  	 //Insert into list of active control connections
  	 connection = (struct DataConn*)malloc(sizeof(struct DataConn));
  	 connection->sockfd = fdaccept;
  	 LIST_INSERT_HEAD(&data_conn_list, connection, next);

     return fdaccept;
 }

 int data_recv_hook(int sock_index){

 }
