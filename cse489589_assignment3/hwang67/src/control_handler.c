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

#include "../include/global.h"
#include "../include/author.h"


/* Linked List for active control connections */
struct ControlConn
{
    int sockfd;
    LIST_ENTRY(ControlConn) next;
}*connection, *conn_temp;

LIST_HEAD(ControlConnsHead, ControlConn) control_conn_list;

// get sockaddr, IPv4 or IPv6:

void *get_in_addr(struct sockaddr *sa) {

    if (sa->sa_family == AF_INET) { return &(((struct sockaddr_in*)sa)->sin_addr); }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

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


int new_control_conn(int sock_index){
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

void remove_control_connection(int sock_index) {
    LIST_FOREACH(connection, &control_conn_list, next) {
        if(connection->sockfd == sock_index) LIST_REMOVE(connection, next); // this may be unsafe?
        free(connection);
    }

    close(sock_index);
}

int isControl(int sock_index){
    LIST_FOREACH(connection, &control_conn_list, next)
        if(connection->sockfd == sock_index) return 1;

    return 0;
}
