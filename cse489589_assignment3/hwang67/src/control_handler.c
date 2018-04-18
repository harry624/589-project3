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
 * This contains the main function. Add further description here....
 */
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <strings.h>
 #include <sys/queue.h>
 #include <unistd.h>
 #include <string.h>

#include "../include/global.h"


int new_control_conn(int sock_index){
   int fdaccpet, sin_size;
   struct sockaddr_storage remoteaddr;  //conntecter's address information;

   sin_size = sizeof remoteaddr;
   new_fd = accept(sock_index, (struct sockaddr *)&remoteaddr, &sin_size);
   if(new_fd == -1){
        perror("accept");
   }

  //Insert into list of active control conections
  return fdaccept;

}


int create_control_socket(){
  int sock;
  int status;
  int new_fd;  // new connection on new_fd
  struct addrinfo hints, *servinfo, *p;
  struct sigaction sa;
  int yes = 1;
  char s[INET6_ADDRSTRLEN];   //incoming connection ip

  memset(&hints, 0, sizeof hints); //make sure the struct is empty
  hints.ai_family = AF_UNSPEC; //don't care IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; //fill in my ip for me

  //get address info
  if ((status = getaddrinfo(NULL, CONTROL_PORT, &hints, &servinfo)) != 0){
      fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
      return 1;
  }
  //loop through all the results and bind to the first we can
  for(p = servinfo; p != NULL; p = p -> ai_next){
      if ((sock = socket(p ->ai_family, p -> ai_socktype, p ->ai_protocol)) == -1){
          perror("server: socket");
          continue;
      }

      if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
          perror("setsockopt");
          exit(1);
      }

      //bind the port
      if (bind(sock, p->ai_addr, p->ai_addrlen) == -1){
          close(sock);
          perror("server: bind");
          continue;
      }
      break;
  }
  freeaddrinfo(servinfo); //add done with this structure

  if(p == NULL) {
      fprintf(stderr, "server: failed to bind\n" );
      exit(1);
  }
  if(listen(sock, BACKLOG) == -1){
      perror("listen");
      exit(1);
  }

  return sock;
}
