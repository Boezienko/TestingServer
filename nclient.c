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
#include <stdbool.h>
#include "packet.h"
#include "connInfo.h"

#define PORT "3456"
#define MAXDATASIZE 100 // max number of bytes we can get at once from server
#define MAXINPUTSIZE 100 // max number of bytes we can get at once for a command
#define MAXINPUTARGS 5 // max number of bytes we can get at once for a command

// declaring functions we will use
ConnInfo* connect_to_server(char* addr);
// sends packet to machine connected to socket file descriptor
void send_packet(int sockfd, Packet* pkt);
// sends whole file to server
void send_file(int sockfd, char* filename);
// gets address IPv4 or IPv6
void *get_in_addr(struct sockaddr *sa);

int main(int argc, char *argv[]){
  ConnInfo* sinfo;
  char input[MAXINPUTSIZE];
  char* inputArgs[MAXINPUTARGS];
  Packet tempPack;
  
  tempPack.initialize = initializePacket;
  
  sinfo = connect_to_server(argv[1]);
  
  // start shell for user input
  while(sinfo != NULL){
    printf("->");
    fgets(input, MAXINPUTSIZE, stdin);
    
    // replace newline character with terminating character
    input[strcspn(input, "\n")] = '\0';
    
    tempPack.initialize(&tempPack, 0, 0, input);
    
    // send input string to server
    send_packet(sinfo->sockfd, &tempPack);
    
    
    // tokenize input into arguments
    char* token = strtok(input, " ");
    int toki = 0;
    while(token != NULL){
      inputArgs[toki] = token;
      token = strtok(NULL, " ");
      toki++;
    }
    
    // do something based on the input
    if(!strcmp(inputArgs[0], "test")){
      printf("Entered test input\n");
      
      // send input string to server
    
    
    } else if(!strcmp(inputArgs[0], "ls-tests")) {
      printf("Entered ls-tests input\n");
      
    } else if(!strcmp(inputArgs[0], "quit")) {
      // done with this structure
      sinfo->free(sinfo);
      printf("Disconnected from server\n");
      sinfo=NULL;
    } else {
      printf("invalid input\n");
    }
  }
  
  
  return 0;
}


/*------------------------Initializing Functions--------------------------------*/

ConnInfo* connect_to_server(char* addr){
  int sockfd;
  struct addrinfo hints, *servinfo, *p;
  int rv;
  char s[INET6_ADDRSTRLEN];
  ConnInfo* connection = (ConnInfo*)malloc(sizeof(ConnInfo));
  connection->initialize = initializeConnInfo;
  
  // ensure hints is clear and read to be filled
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  
  // get list of addrinfo structs to create socket descriptors
  if ((rv = getaddrinfo(addr, PORT, &hints, &servinfo)) != 0){
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    exit(1);
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
    exit(1);
  }
  
  // prep structure to return with info it needs
  connection->initialize(connection, servinfo, sockfd);
  
  // check server IP we connected to
  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
  printf("client: connecting to %s\n", s);
  
  return connection;
}

// to make sending packets easier
void send_packet(int sockfd, Packet* pkt){
  char* spkt = pkt->serialize(pkt);
  
  printf("\nSending:\n%s\n", spkt);
  
  
  if(send(sockfd, spkt, strlen(spkt), 0) == -1){
    perror("send");
  }
  free(spkt);
  // so multiple sends streams aren't interpereted as one
  sleep(1);
}

// to send messages easier and encapsulate error handling
void send_file(int sockfd, char* filename){
  
  FILE* file = fopen(filename, "r");
  if(file == NULL){
    perror("fopen");
    return;
  }
  
  Packet packet;
  char pktBuf[MAXDATASIZE];
  size_t numBytes;
  int seqNum = 0;
  
  // prepare packet
  packet.initialize = initializePacket;
  
  while((numBytes = fread(pktBuf, 1, MAXDATASIZE-1, file)) > 0){
    pktBuf[numBytes] = '\0';
    packet.initialize(&packet, 2, seqNum, pktBuf);
    send_packet(sockfd, &packet);
    packet.free(&packet);
    seqNum++;
  }
  
  // send EOF packet
  packet.initialize(&packet, 3, seqNum, "EOF");
  send_packet(sockfd, &packet);
  packet.free(&packet);
  
  fclose(file);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa){
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
