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
 * Network I/O utility functions. send/recvALL are simple wrappers for
 * the underlying send() and recv() system calls to ensure nbytes are always
 * sent/received.
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <errno.h>
 #include <string.h>
 #include <sys/types.h>
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <arpa/inet.h>
 #include <netdb.h>

#include "../include/global.h"

int create_send_UDP_socket(){
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0)
        perror("socket() failed");
	return sock;
}


ssize_t sendtoALL(int sock_index, char *buffer, ssize_t nbytes, struct sockaddr_in to){

    socklen_t tolen;
    tolen = sizeof to;
    ssize_t bytes = 0;
    bytes = sendto(sock_index, buffer, nbytes, 0, (struct sockaddr*)&to, tolen);

    while (bytes != nbytes) {
          bytes += sendto(sock_index, buffer+bytes, nbytes - bytes, 0, (struct sockaddr*)&to, tolen);
    }
    return bytes;
}

ssize_t recvfromALL(int sock_index, char *buffer, ssize_t nbytes){
    struct sockaddr_storage from;
    socklen_t fromlen;
    fromlen = sizeof from;
    ssize_t bytes = 0;
    bytes = recvfrom(sock_index, buffer, nbytes, 0, (struct sockaddr *)&from, &fromlen);

    if (bytes == 0){
      return 0;
    }
    while (bytes != nbytes) {
       bytes += recvfrom(sock_index, buffer+bytes, nbytes-bytes, 0, (struct sockaddr *)&from, &fromlen);
    }
    return bytes;
}

ssize_t recvALL(int sock_index, char *buffer, ssize_t nbytes)
{
    ssize_t bytes = 0;
    bytes = recv(sock_index, buffer, nbytes, 0);

    printf("receive bytes: %d\n", bytes);

    if(bytes == 0) return -1;
    while(bytes != nbytes)
        bytes += recv(sock_index, buffer+bytes, nbytes-bytes, 0);

    return bytes;
}

ssize_t sendALL(int sock_index, char *buffer, ssize_t nbytes)
{
    ssize_t bytes = 0;
    bytes = send(sock_index, buffer, nbytes, 0);

    if(bytes == 0) return -1;
    while(bytes != nbytes)
        bytes += send(sock_index, buffer+bytes, nbytes-bytes, 0);


    printf("sent bytes: %d\n", bytes);
    return bytes;
}
