#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "packet.h"

#define PORT "3456"
#define SERVER_IP "144.96.151.55"
#define MAXDATASIZE 100 // max number of bytes we can get at once

// to send messages easier and encapsulate error handling
void send_(int sockfd, int flag, char* msg){
  Packet packet;
  char pktBuf[MAXDATASIZE];
  
  // prepare packet
  packet.initialize = initializePacket;
  
  // give data to packet
  packet.initialize(&packet, flag, msg);
  packet.serialize(&packet, pktBuf);
  
  // send packet
  if(send(sockfd, pktBuf, strlen(pktBuf), 0) == -1){
    perror("send");
  }
  packet.free(&packet);
  
  // so that two sends do not send as part of the same stream
  sleep(1);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa){
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]){
  int sockfd, numbytes;
  char buf[MAXDATASIZE];
  struct addrinfo hints, *servinfo, *p;
  int rv;
  char s[INET6_ADDRSTRLEN];
  
  // make sure correct number of expected arguments
  if(argc != 3){
    fprintf(stderr, "ussage: client filename testecase_filename\n");
    return 1;
  }
  
  // ensure hints is clear and read to be filled
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  
  // get list of addrinfo structs to create socket descriptors
  if ((rv = getaddrinfo(SERVER_IP, PORT, &hints, &servinfo)) != 0){
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }
  
  // loop through all the results and connect to the first one we can
  for (p = servinfo; p != NULL; p = p->ai_next){
  
    // create the socket
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
      perror("client: socket");
      // if error encountered, try creating socket with next addrinfo struct
      continue;
    }
    
    // connect to server
    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("client: connect");
      // if error encountered, try again with next result
      continue;
    }
    
    // we have successfully created and connected to the socket, so leave loop
    break;
  }
  
  if (p == NULL) {
    fprintf(stderr, "client: failed to connect\n");
    return 2;
  }
  
  // check server IP we connected to
  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
  printf("client: connecting to %s\n", s);
  
  // sending file to test's name
  send_(sockfd, 1, argv[1]);


  
  // sending testcase file's name
  send_(sockfd, 1, argv[2]);
  
  // clear buffer before using
  memset(&buf, 0, sizeof buf);
  
  // read in file that we need to read in, as we're reading it in, we will be sending it
  FILE* file = fopen(argv[1], "r");
  if(file == NULL){
    perror("fopen");
    close(sockfd);
    return 1;
  }
  
  // variables to use to read in file
  size_t bytesRead;
  
  // read in file in chunks, then send, repeat until nothing is read in
  while((bytesRead = fread(buf, 1, MAXDATASIZE - 1, file)) > 0){
    send_(sockfd, 2, buf);
    // clear buffer for reuse
    memset(buf, 0, sizeof buf);
  }
  
  // signaling end of file
  send_(sockfd, 3, "EOF");
  
  // receive message from server as to weather the program passed or failed
  
  // print message recieved from server
  
  // freeing up this structure
  freeaddrinfo(servinfo);
  fclose(file);
  close(sockfd);
  
  return 0;
}
