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
 * Connection Manager listens for incoming connections/messages from the
 * controller and other routers and calls the desginated handlers.
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

#include "../include/global.h"
#include "../include/connection_manager.h"
#include "../include/control_handler.h"
#include "../include/data_handler.h"
#include "../include/control_header_lib.h"
#include "../include/routing_handler.h"
#include "../include/network_util.h"



 void main_loop(/* arguments */) {
     int fdaccpet;

     //init timer
     struct timeval tv;
     boardcast_interval = 5;
     tv.tv_sec = boardcast_interval;
     tv.tv_usec = 0;

     int timeoutIndex = -1;
     while(1){
         read_fds = master; // copy it

         //timer should be updated Here, check the find the earliest timeout
         //find the earliest timeout time and index
         int diff = INF;
         for (int i = 0; i < num_neighbors; i++){
            if (neighbors[i] == 1 && routers[i].firstupdateReceived == 1 && routers[i].isRemoved == 0){
                int timenow = time(0);
                if ((timerArray[i] - timenow) < diff){
                    diff = timerArray[i] - timenow;
                    timeoutIndex = i;
                    tv.tv_sec = diff;
                    printf("next timeout index is %d, remaining time is %d\n",i, diff);
                }
            }
         }

         //if find the earlist timer, update tv

         int res = select(fdmax+1, &read_fds, NULL, NULL, &tv);

         if (res == -1) {
             perror("select");
             exit(4);
         }else if (res > 0){

             // run through the existing connections looking for data to read
             for(int i = 0; i <= fdmax; i++) {

                 if(FD_ISSET(i, &read_fds)){
                    // printf("control_socket: %d, router_socket: %d, data_socket:%d\n",FD_ISSET(control_socket, &read_fds), FD_ISSET(router_socket, &read_fds), FD_ISSET(data_socket, &read_fds));
                     //control socket
                     if(i == control_socket){
                        fdaccpet = new_control_conn(i);
                        printf("new control conntection: %d\n", fdaccpet);
                        //add to master fd
                        FD_SET(fdaccpet, &master);
                        if(fdaccpet > fdmax) fdmax = fdaccpet;
                     }
                     //router socket
                     else if (i == router_socket){
                       // printf("handle router_socket: %d\n", i);
                       int router_index = recv_update_distanceVector(i);

                     }
                     //data socket
                     else if (i == data_socket){
                         fdaccpet = new_data_conn(i);
                         printf("new data conntection: %d\n", fdaccpet);
                         //add to master fd
                         FD_SET(fdaccpet, &master);
                         if(fdaccpet > fdmax) fdmax = fdaccpet;

                         handle_data(fdaccpet);
                      }
                     //existing socket
                     else{
                          if (isControl(i)){
                              int res = control_recv_hook(i);
                              if(!res) FD_CLR(i, &master);
                          }
                          else if(isData(i)){
                              // if(!data_recv_hook(i)){
                              //     FD_CLR(i, &master);
                              // }
                              if(!handle_data(i)){
                                  FD_CLR(i, &master);
                              }
                          }
                          else{
                            //unknown socket
                            perror("uknown socket");
                          }
                     }
                 }
             }
         }else if (res == 0){
             tv.tv_sec = boardcast_interval;
             tv.tv_usec = 0;

             // add missed count;
             if (timeoutIndex != -1){
                routers[timeoutIndex].missedcnt++;
                timerArray[timeoutIndex] = time(0) + boardcast_interval;
                printf("index %d miss count: %d\n", timeoutIndex, routers[timeoutIndex].missedcnt);

             }

             // check if missed count > 3 time period
             for (int i = 0; i < num_neighbors; i++){
                if (neighbors[i] == 1 && routers[i].missedcnt >= 3 && routers[i].isRemoved == 0){
                    // printf("lost connection between local: %d and neighbors: %d\n", localRouterIndex, i);
                    // count++;
                    distanceVector[localRouterIndex][i] = INF;
                    routers[i].cost = INF;
                    routers[i].nextHopID = INF;
                    routers[i].isRemoved = 1;
                }
             }

             // updated_num_neighbors = updated_num_neighbors - count;

             updateDVBybellmanFord();

             int sock = create_send_UDP_socket();
             boardcast_update_routing(sock, neighbors, routers);
         }
     }
 }

 void init() {
     control_socket = create_control_socket();

     //router_socket and data_socket will be initialized after INIT from controller

     // clear the master and temp sets
     FD_ZERO(&master);
     FD_ZERO(&read_fds);

     //Register the control socket
     FD_SET(control_socket, &master);

     fdmax = control_socket;

     main_loop();

 }
