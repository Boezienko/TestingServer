#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

typedef struct ConnInfo{
  struct addrinfo* addr_info;
  int sockfd;
  
  void (*initialize)(struct ConnInfo*, struct addrinfo*, int);
  void (*free)(struct ConnInfo*);  
} ConnInfo;

// declaring functions
void initializeConnInfo(ConnInfo* conn, struct addrinfo* sinfo, int sockfd);
void freeConnInfo(ConnInfo* conn);

// initializing functions
void initializeConnInfo(ConnInfo* conn, struct addrinfo* sinfo, int sockfd){
  conn->addr_info = sinfo;
  conn->sockfd = sockfd;
  conn->initialize = initializeConnInfo;
  conn->free = freeConnInfo;
}

void freeConnInfo(ConnInfo* conn){
  freeaddrinfo(conn->addr_info);
  close(conn->sockfd);
  free(conn);
}
