#ifndef PACKET_H
#define PACKET_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAXDATASIZE 100 // size of packet
// this is our packet structure that we will use to help facilitate the sending of messages from machine to machine
// each packet will have a flag to indicate what kind of packet it is
// each packet will have a payload, which is it's actual message

typedef struct Packet {
    int flag; // 1 for command, 2 for datafile, 3 for last datafile in transmission
    int length; // length of payload
    int seqNum; // sequence number of packet
    char* payload; // message inside packet
    
    // pointer functions for packets to use
    char* (*serialize)(struct Packet*);
    void (*deserialize)(struct Packet*, char*);
    void (*initialize)(struct Packet*, int, int, char*);
    void (*free)(struct Packet*);    
} Packet;

// declaring functions
char* serialize(Packet* pkt);
void deserialize(Packet* pkt, char* strpkt);
void initializePacket(Packet* pkt, int flag, int seq, char* payload);
void freePacket(Packet* pkt);

// convert the packet to a sendable string
char* serialize(Packet* pkt){
  int initial_size = MAXDATASIZE;
  int required_size;
  char* strpkt = (char*)malloc(initial_size);
  
  while (1) {
    int n = snprintf(strpkt, initial_size, "%d-%d-%d-%s", pkt->flag, pkt->length, pkt->seqNum, pkt->payload);

    // Check if the buffer was large enough
    if (n >= 0 && n < initial_size) {
      // Serialization was successful
      return strpkt;
    }

    // Buffer was too small, calculate required size
    if (n < 0) {
      perror("snprintf error");
      free(strpkt);
      exit(EXIT_FAILURE);
    }

    required_size = n + 1; // +1 for the null terminator

    // Reallocate buffer with the required size
    char* temp = (char*)realloc(strpkt, required_size);
    if (temp == NULL) {
      perror("Failed to reallocate memory");
      free(strpkt);
      exit(EXIT_FAILURE);
    }

    strpkt = temp;
    initial_size = required_size;
  }
}

// converts the packets sendable string data into the data being held in our packet instance
void deserialize(Packet* pkt, char* strpkt){
  char* token;
  
  printf("received: %s\n", strpkt);
  
  // extract the flag
  token = strsep(&strpkt, "-");
  if(token == NULL){
    printf("\n--------------------------\nFlag: Invalid packet format\n--------------------------\n");
    printf("received: %s\n", strpkt);
    exit(1);
  }
  pkt->flag = atoi(token);
  
  // extract the length
  token = strsep(&strpkt, "-");
  if(token == NULL){
    printf("\n--------------------------\nLength: Invalid packet format\n--------------------------\n");
    printf("received: %s\n", strpkt);
    exit(1);
  }
  pkt->length = atoi(token);
  
  // extract the sequence number
  token = strsep(&strpkt, "-");
  if(token == NULL){
    printf("\n--------------------------\nSequenceNumber: Invalid packet format\n--------------------------\n");
    printf("received: %s\n", strpkt);
    exit(1);
  }
  pkt->seqNum = atoi(token);
  
  // free existing payload memory
  /*
  if (pkt->payload){
    free(pkt->payload);
  }
  */
  
  // extract the payload
  if(pkt->length == 0){
    pkt->payload = NULL;
  } else if (pkt->length > 0){
    token = strpkt;
    pkt->payload = (char*)malloc(pkt->length + 1);
    strncpy(pkt->payload, token, pkt->length);
    pkt->payload[pkt->length] = '\0';
  } else {
    printf("Payload length is negative. Invalid\n");
    exit(1);
  }

}

void initializePacket(Packet* pkt, int flag, int seqNum, char* payload){
  pkt->flag = flag;
  pkt->length = strlen(payload);
  pkt->seqNum = seqNum;
  pkt->payload = (char*)malloc(pkt->length + 1);
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
