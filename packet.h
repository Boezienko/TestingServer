#ifndef PACKET_H
#define PACKET_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAXDATASIZE 100 // size of payload
#define MAXPACKSIZE 105 // size of packet
// this is our packet structure that we will use to help facilitate the sending of messages from machine to machine
// each packet will have a flag to indicate what kind of packet it is
// each packet will have a payload, which is it's actual message

typedef struct Packet {
    int flag; // 0 for user input, 1 for file contents, 2 for last packet in file transmission
    char* payload; // message inside packet
    
    // pointer functions for packets to use
    char* (*serialize)(struct Packet*);
    void (*deserialize)(struct Packet*, char*);
    void (*initialize)(struct Packet*, int, char*);
    void (*free)(struct Packet*);    
} Packet;

// declaring functions
char* serialize(Packet* pkt);
void deserialize(Packet* pkt, char* strpkt);
void initializePacket(Packet* pkt, int flag, char* payload);
void freePacket(Packet* pkt);

// convert the packet to a sendable string
char* serialize(Packet* pkt){  
  char* strpkt = (char*)malloc(MAXPACKSIZE);
  memset(strpkt, 0, MAXPACKSIZE);
  snprintf(strpkt, MAXPACKSIZE, "%d-%s", pkt->flag, pkt->payload);
  return strpkt;
}

// converts the packets sendable string data into the data being held in our packet instance
void deserialize(Packet* pkt, char* strpkt){
  char* token;
  // copy original string
  char* strpktcpy = strdup(strpkt);
  if(!strpktcpy){
    perror("strdup");
    exit(EXIT_FAILURE);
  }
  
  //printf("received: %s\n", strpkt);
  
  // extract the flag
  token = strtok(strpktcpy, "-");
  if(token == NULL){
    printf("Invalid packet format\n");
    //printf("received: %s\n", strpkt);
    free(strpktcpy);
    exit(1);
  }
  pkt->flag = atoi(token);
  
  // extract the payload
  token = strtok(NULL, "");
  if(token != NULL) {
    pkt->payload = (char*)malloc(strlen(token) + 1);
    strcpy(pkt->payload, token);
  } else {
    // in case payload is empty
    pkt->payload = strdup("");
  }
  
  free(strpktcpy);
}

void initializePacket(Packet* pkt, int flag, char* payload){
  pkt->flag = flag;
  pkt->payload = (char*)malloc(strlen(payload) + 1);
  strcpy(pkt->payload, payload);
  pkt->serialize = serialize;
  pkt->deserialize = deserialize;
  pkt->initialize = initializePacket;
  pkt->free = freePacket;
}

void freePacket(Packet* pkt){
  if (pkt->payload){
    free(pkt->payload);
    pkt->payload = NULL;
  }
}

#endif
