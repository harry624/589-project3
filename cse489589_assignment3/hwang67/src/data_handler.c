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


 void create_data_socket(uint16_t dataPort) {
     printf("create_data_socket\n");
     int sock;
     struct sockaddr_in data_addr;
     socklen_t addrlen = sizeof(data_addr);

     sock = socket(AF_INET, SOCK_STREAM, 0);
     if(sock < 0){
       perror("server: socket");
     }
     /* Make socket re-usable */
     if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) < 0){
         perror("setsockopt");
         exit(1);
     }
     if(setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, (int[]){1}, sizeof(int)) < 0){
         perror("setsockopt");
         exit(1);
     }
     bzero(&data_addr, sizeof(data_addr));

     data_addr.sin_family = AF_INET;
     data_addr.sin_addr.s_addr = htonl(INADDR_ANY);
     data_addr.sin_port = htons(dataPort);

     if(bind(sock, (struct sockaddr *)&data_addr, sizeof(data_addr)) < 0){
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
     // FD_SET(data_socket, &read_fds);

     if(data_socket > fdmax) fdmax = data_socket;

     printf("data_socket: %d, fdmax: %d, is in the list: %d\n", data_socket, fdmax, FD_ISSET(data_socket, &master));

     return;
 }

//0x05 send file
void sending_file(char *filename) {

    return;
}

void findNextHop(uint32_t destIP) {
    int destationID;
    for(int i = 0; i < num_neighbors; i++){
      if (destIP == routers[i].int32_ip){
          destationID = routers[i].routerID;
          break;
      }
    }
    return;
}

void send_file(int sock_index, char * cntrl_payload, uint16_t payload_len){
    uint32_t tempdestIp;
    uint8_t init_TTL;
    uint8_t transferID;
    uint16_t init_seq_num;

    char *filename;
    char destIP[40];

    struct SEND_FILE_CONTROL *send_file = (struct SEND_FILE_CONTROL *) cntrl_payload;
    tempdestIp = ntohl(send_file->destationIP);
    sprintf(destIP, "%d.%d.%d.%d", ((tempdestIp>>24)&((1<<8)-1)), ((tempdestIp>>16)&((1<<8)-1)), ((tempdestIp>>8)&((1<<8)-1)), (tempdestIp&((1<<8)-1)));

    init_TTL = ntohs(send_file->init_TTL);
    transferID = ntohs(send_file->transferID);
    init_seq_num = ntohs(send_file->init_seq_num);

    //to-do get filename
    int file_name_len = payload_len - 8 + 1;
    filename = (char*)malloc(file_name_len); // for remaining size, 1 for null terminator
    memset(filename, 0, file_name_len);
    strncpy(filename, cntrl_payload+8, file_name_len-1);
    filename[file_name_len] = '\0';

    int file_len=0;
  	int loop = 0;

    FILE *ptr_file;
    char buf[1024];
    // https://www.linuxquestions.org/questions/programming-9/c-howto-read-binary-file-into-buffer-172985/
    ptr_file = fopen(filename, "rb");
    if(ptr_file == NULL){
      char* cntrl_response_header = create_response_header(sock_index, 5, 0, 0);
      sendALL(sock_index, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
      return;
    }

    fseek(ptr_file, 0, SEEK_END);
    file_len = ftell(ptr_file);
    fseek(ptr_file, 0, SEEK_SET);
    loop = file_len/1024;

    findNextHop(tempdestIp);

    sending_file(filename);

    return;
}

void remove_data_conn(int sock_index){
    printf("remove_data_conn:%d\n", sock_index);
    LIST_FOREACH(connection, &data_conn_list, next) {
        if(connection->sockfd == sock_index) LIST_REMOVE(connection, next); // this may be unsafe?
        free(connection);
    }

    close(sock_index);
}

int isData(int sock_index){
  	//cout<<"Is Data"<<endl;
  	 LIST_FOREACH(connection, &data_conn_list, next)
  		 if(connection->sockfd == sock_index) return 1;
  	 return 0;
 }

int new_data_conn(int sock_index) {
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
     printf("is control_recv_hook: %d\n", sock_index);
     char *data_header, *data_payload;


     /* Get control header */
     data_header = (char *) malloc(sizeof(char)*DATA_PACKET_HEADER_SIZE);
     bzero(data_header, DATA_PACKET_HEADER_SIZE);

     if(recvALL(sock_index, data_header, DATA_PACKET_HEADER_SIZE) < 0){
         remove_data_conn(sock_index);
         free(data_header);
         return 0;
     }

 }
