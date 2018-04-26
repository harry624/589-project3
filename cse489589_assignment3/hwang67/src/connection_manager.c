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
#include "../include/control_header_lib.h"
#include "../include/routing_handler.h"



 void main_loop(/* arguments */) {
     int fdaccpet;

     struct timeval tv;
     boardcast_interval = 0;
     tv.tv_sec = boardcast_interval;

     while(1){
         read_fds = master; // copy it
         if (select(fdmax+1, &read_fds, NULL, NULL, &tv) == -1) {
             perror("select");
             exit(4);
         }
       // run through the existing connections looking for data to read
         for(int i = 0; i < fdmax + 1; i++) {
              // printf("router_socket is in master:%d\n", FD_ISSET(router_socket, &read_fds));
             if(FD_ISSET(i, &read_fds)){
                  printf("main loop socket:%d, fdmax: %d, router_socket: %d, is in the list: %d\n", i, fdmax, router_socket, FD_ISSET(router_socket, &read_fds));
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
                     printf("handle router_socket: %d\n", i);
                     boardcast_update_routing(i, neighbors, routers);
                     
                     recv_update_distanceVector(i);
                 }
                 //data socket
                 else if (i == data_socket){
                    printf("handle data_socket: %d\n", i);

                 }
                 //existing socket
                 else{
                      if (isControl(i)){
                          int res = control_recv_hook(i);
                          if(!res) FD_CLR(i, &master);
                      }
                      else{
                        //unknown socket
                        perror("Unknown socket index");
                      }
                 }
             }
         }
     }
 }

 void init() {
   control_socket = create_control_socket();

   // printf("init socket created:%d\n", control_socket);
   //router_socket and data_socket will be initialized after INIT from controller

   // clear the master and temp sets
   FD_ZERO(&master);
   FD_ZERO(&read_fds);

   //Register the control socket
   FD_SET(control_socket, &master);

   fdmax = control_socket;

   main_loop();

 }
