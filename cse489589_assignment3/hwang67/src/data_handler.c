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

//
int createSocketToNextRouter(uint32_t destIp, uint16_t dataPort){
    int sock;
    struct sockaddr_in data_addr;
    socklen_t addrlen = sizeof(data_addr);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0){
      perror("server: socket");
      return -1;
    }
    bzero(&data_addr, sizeof(data_addr));

    data_addr.sin_family = AF_INET;
    data_addr.sin_addr.s_addr = htonl(destIp);
    data_addr.sin_port = htons(dataPort);

    if(connect(sock, (struct sockaddr*)&data_addr, sizeof data_addr) == -1){
        perror("connect failed");
        return -1;
    }
    return sock;
}

//listener
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
char* get_File_Name(uint16_t transfer_id){

    // number of digits
    int num;
	if(transfer_id > 100){
		num = 3;
	}else if(transfer_id > 10){
		num = 2;
	}else{
		num = 1;
	}

    // 5 for 'file-' and one for null
    char* file_name = (char*)malloc(6+sizeof(char)*num);
    strcpy(file_name, "file-");
    sprintf(file_name+5, "%d", transfer_id);
    file_name[strlen(file_name)] = '\0';
    // return file_name;
    return file_name;

}

uint16_t findNextHop(uint32_t destIP) {
    int destationID;
    for(int i = 0; i < num_neighbors; i++){
      if (destIP == routers[i].int32_ip){
          return routers[i].nextHopID;
      }
    }
    return -1;
}

int sending_file(uint32_t destIP, uint8_t transferID, uint8_t TTL, uint16_t seq_num, int finbit, char* file) {
    //find next hop
    uint16_t next_hop_id;
    int next_hop_index = -1;
    next_hop_id = findNextHop(destIP);

    for (int i = 0; i < num_neighbors; i++){
        if(next_hop_id == routers[i].routerID){
            next_hop_index = i;
        }
    }

    if (next_hop_id == -1){
        return -1;
    }

    //pack sending packet
    char *file_packet;
    file_packet = (char *) malloc(DATA_PACKET_SIZE);
    uint16_t FIN     = 0x00;
    uint16_t padding = 0x00;
    destIP = htonl(destIP);
    seq_num = htons(seq_num);

    memcpy(file_packet, &destIP, sizeof(destIP));
    memcpy(file_packet + 4, &transferID, sizeof(transferID));
    memcpy(file_packet + 5, &TTL, sizeof(TTL));
    memcpy(file_packet + 6, &seq_num, sizeof(seq_num));

    if (finbit == 1){
        FIN = htons(1);
    }else{
        FIN = htons(0);
    }
    memcpy(file_packet + 8, &FIN, sizeof(FIN));
  	memcpy(file_packet + 9, &padding, sizeof(padding));
  	memcpy(file_packet + 12, file, 1024);

    //save the penultimateDataPacket and ultimateDataPacket
    memcpy(penultimateDataPacket, ultimateDataPacket, DATA_PACKET_SIZE);
  	memcpy(ultimateDataPacket, file_packet, DATA_PACKET_SIZE);
    //check if it is the first router
    if(fileStatArray[transferID].transfer_id != transferID){
      	fileStatArray[transferID].first_seq_num = ntohs(seq_num);
      	printf("first seq num: %d\n", ntohs(seq_num));
      	fileStatArray[transferID].ttl = TTL;
      	fileStatArray[transferID].transfer_id = transferID;
      	fileStatArray[transferID].index = 0;
      }

      // insert sequence number
      fileStatArray[transferID].seq_num_array[fileStatArray[transferID].index++] = seq_num;

      int data_Sock = routers[next_hop_index].data_socket_fd;
      if(data_Sock == 0){
        	data_Sock = createSocketToNextRouter(routers[next_hop_index].int32_ip, routers[next_hop_index].dataPort);
        	routers[next_hop_index].data_socket_fd = data_Sock;
      }

      if (data_Sock > 0){
        sendALL(data_Sock, file_packet, DATA_PACKET_SIZE);
      }
      if(finbit == 1){
          //close the socket
          close(data_Sock);
          printf("closed\n");
          routers[next_hop_index].data_socket_fd = 0;
      }

      printf("free payload\n");
      free(file_packet);
      return 1;
}

void handle_data(int sock_index){
    char file_data[1024];
    char* file_packet = (char *) malloc(sizeof(char)*DATA_PACKET_SIZE);
    bzero(file_packet, DATA_PACKET_SIZE);

    //receive packet
    if(recvALL(sock_index, file_packet, DATA_PACKET_SIZE) < 0){
        close(sock_index);
        FD_CLR(sock_index, &master);
    }

    uint32_t dest_ip;
    uint8_t transfer_id;
    uint8_t ttl;
    uint16_t seq_num;
    uint16_t FIN;
    int finbit = 0;

    struct DATA_PACKET *packet = (struct DATA_PACKET *) file_packet;

    dest_ip     = ntohl(packet->dest_ip);
    transfer_id = packet->transfer_id;
    ttl         = packet->ttl;
    seq_num     = ntohs(packet->seq_no);
    FIN         = ntohs(packet->FIN);
    memcpy(packet + 12, file_data, 1024);

    ttl = ttl-1;

    if (FIN == 1){
      finbit = 1;
    }
    if (ttl > 0){
        //check if it is the destination
        if (routers[localRouterIndex].int32_ip != dest_ip){
            //send file to another router
            sending_file(dest_ip, transfer_id, ttl, seq_num, finbit, file_data);
        }else{
            //it is the destination, save file
            char* file_name = get_File_Name(transfer_id);

            if(fileStatArray[transfer_id].transfer_id != transfer_id){
                  fileStatArray[transfer_id].fptr = fopen(file_name, "wb");
            }

            printf("byteswritten:%d\n",fwrite(file_data, 1, 1024, fileStatArray[transfer_id].fptr));

                // this is the first packet
            if(fileStatArray[transfer_id].first_seq_num == 0){
                  printf("receiving file:\n");
                  fileStatArray[transfer_id].first_seq_num = seq_num;
                  fileStatArray[transfer_id].ttl = ttl;
                  fileStatArray[transfer_id].transfer_id = transfer_id;
                  fileStatArray[transfer_id].index = 0;
            }

            // insert sequence number
            fileStatArray[transfer_id].seq_num_array[fileStatArray[transfer_id].index++] = htons(seq_num);

            if(finbit == 1){
                //fileStatArray[transfer_id].last_seq_num = seq_num;
                fclose(fileStatArray[transfer_id].fptr);
                fileStatArray[transfer_id].fptr = NULL;
            }

            //save penultimateDataPacket
            memcpy(penultimateDataPacket, ultimateDataPacket, DATA_PACKET_SIZE);

            //save ultimateDataPacket
            dest_ip = htonl(dest_ip);
            seq_num = htons(seq_num);
            uint16_t FIN = htons(FIN);
            uint16_t padding = 0x00;

            memcpy(ultimateDataPacket, &dest_ip, sizeof(dest_ip));
            memcpy(ultimateDataPacket + 4, &transfer_id, sizeof(transfer_id));
            memcpy(ultimateDataPacket + 5, &ttl, sizeof(ttl));
            memcpy(ultimateDataPacket + 6, &seq_num, sizeof(seq_num));
            memcpy(ultimateDataPacket + 8, &FIN, sizeof(FIN));
            memcpy(ultimateDataPacket + 9, &padding, sizeof(padding));
            memcpy(ultimateDataPacket + 12, file_data, 1024);
        }
    }

    if(finbit == 1){
        FD_CLR(sock_index, &master);
        close(sock_index);
    }
}

void send_file(int sock_index, char * cntrl_payload, uint16_t payload_len){
    uint32_t tempdestIp;
    uint8_t init_TTL;
    uint8_t transferID;
    uint16_t seq_num;

    char destIP[40];

    struct SEND_FILE_CONTROL *send_file = (struct SEND_FILE_CONTROL *) cntrl_payload;
    tempdestIp = ntohl(send_file->destationIP);
    sprintf(destIP, "%d.%d.%d.%d", ((tempdestIp>>24)&((1<<8)-1)), ((tempdestIp>>16)&((1<<8)-1)), ((tempdestIp>>8)&((1<<8)-1)), (tempdestIp&((1<<8)-1)));

    init_TTL    = send_file->init_TTL;
    transferID = send_file->transferID;
    seq_num = ntohs(send_file->init_seq_num);


    //get filename
    char *filename;
    int file_name_len = payload_len - 8 + 1;
    filename = (char*)malloc(file_name_len); // for remaining size, 1 for null terminator
    memset(filename, 0, file_name_len);
    strncpy(filename, cntrl_payload+8, file_name_len-1);
    filename[file_name_len] = '\0';

    printf("destIp: %s, ttl: %d, transferID: %d, seq_num: %d, file_name: %s\n", destIP, init_TTL, transferID, seq_num, filename);

    if(init_TTL <= 0){
  		  return;
  	}

    //read binary file into data
    int file_len = 0;
  	int totalPacketsToBeSent = 0;

    FILE *file_pointer;
    char file_buffer[1024];

    file_pointer = fopen(filename, "rb");

    if(file_pointer == NULL){
        return;
    }

    fseek(file_pointer, 0, SEEK_END);
  	file_len = ftell(file_pointer);
  	fseek(file_pointer, 0, SEEK_SET);
  	totalPacketsToBeSent = file_len / 1024;

  	int finbit = 0;
    int file_stat = 0;

  	for(int i=0; i < totalPacketsToBeSent; i++){
    		if(i == totalPacketsToBeSent - 1){
      			// the last packet
      			finbit = 1;
  		  }

    		fseek(file_pointer, 0, SEEK_CUR);

    		printf("bytesread:%d\n",fread(file_buffer, 1, 1024, file_pointer));

        file_stat = sending_file(tempdestIp, transferID, init_TTL, seq_num, finbit, file_buffer);

        if(file_stat == -1){
            fclose(file_pointer);
            return;
        }
        seq_num++;
    }
    fclose(file_pointer);

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
