#ifndef PACKET_H
#define PACKET_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// this is our packet structure that we will use to help facilitate the sending of messages from machine to machine
// each packet will have a flag to indicate what kind of packet it is
// each packet will have a payload, which is it's actual message

typedef struct Packet {
    int flag; // 1 for command, 2 for datafile, 3 for last datafile in transmission
    int length;
    char* payload; // message inside packet
    
    // pointer functions for packets to use
    void (*serialize)(struct Packet*, char*);
    void (*initialize)(struct Packet*, int, char*);
    void (*free)(struct Packet*);    
} Packet;

// declaring functions
void serialize(Packet* pkt, char* strpkt);
void initializePacket(Packet* pkt, int flag, char* payload);
void freePacket(Packet* pkt);

// convert the packet to a sendable string
void serialize(Packet* pkt, char* strpkt){
    sprintf(strpkt, "%d-%d-%s", pkt->flag, pkt->length, pkt->payload);
}

void initializePacket(Packet* pkt, int flag, char* payload){
  pkt->flag = flag;
  pkt->length = strlen(payload);
  pkt->payload = (char*)malloc(pkt->length + 1);
  strcpy(pkt->payload, payload);
  pkt->serialize = serialize;
  pkt->initialize = initializePacket;
  pkt->free = freePacket;
}

void freePacket(Packet* pkt){
  free(pkt->payload);
}

#endif
