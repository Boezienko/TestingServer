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
#include <ctype.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <string.h>
#include <inttypes.h>

#define PORT "3456"
#define BACKLOG 10// how many pending connections queue will hold
#define MAXDATASIZE 100 // max number of bytes we can get at once

// function for theads to run to handle clients
void* handle_client(void* arg){
  printf("Handling thread entered\n");
  FILE* fp;
  bool doneReceiving = false;
  int numbytes = 0;
  int new_fd = *((int*)arg);
  free(arg); // dealocating memory allocated for clients file descriptor
  char buf[MAXDATASIZE];
  memset(&buf, 0, sizeof buf); // clear buffer before using
  
  char filename[256 + MAXDATASIZE];
  
  // populating filename with threadId
  sprintf(filename, "%"PRIu64, (uint64_t)pthread_self());
  
  // get filename from client
  if((numbytes = recv(new_fd, buf, MAXDATASIZE - 1, 0)) == -1){
      perror("recv");
      exit(1);
  }
  
  // show what filename we're testing
  printf("Filename to test: %s\n", buf);
  
  // making it so filename is (whatever name was sent)+threadId, so if multiple users send files with the same name, they will be unique
  strcat(filename, buf);
  
  // create file and prepare to write to it
  fp = fopen(filename, "w");
  if(fp == NULL){
    perror("Error creating file");
    exit(1);
  }
  
  // the issue we are having getting the testcase is that 
  // get testcase file name
  memset(&buf, 0, sizeof buf); // clear buffer before using
  if((numbytes = recv(new_fd, buf, MAXDATASIZE - 1, 0)) == -1){
      perror("recv");
      exit(1);
  }
  printf("Testcase filename: %s\n", buf);
  
  // get file from client
  while(!doneReceiving){
    printf("Entered while loop\n");
    // recieve message that can fit in buffer
    if((numbytes = recv(new_fd, buf, MAXDATASIZE - 1, 0)) == -1){
      perror("recv");
      exit(1);
    }
    
    // print what was received
    for(int i = 0; i < MAXDATASIZE; i++){
      printf("%c", buf[i]);
      fprintf(fp,"%c", buf[i]);
      if(buf[i] == EOF){
        doneReceiving = true;
        fclose(fp);
        break;
      }
    }
    
    if(doneReceiving){
      break;
    }
    
    // clear buffer for reuse
    
    // need to delete later when sending entire file
  }
  
  // use execvp to run their code

  close(new_fd);
  printf("\nThread done\n");
  return NULL;
}

// to
void recv_(int sockfd, char* buf){
  memset(&buf, 0, sizeof buf); // clear buffer before using
  if((recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1){
      perror("recv");
      exit(1);
  }
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa){
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void){
  int sockfd; // listen on sock_fd
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr; // connector's address information
  socklen_t sin_size;
  //struct sigaction sa;
  int yes=1;
  char s[INET6_ADDRSTRLEN];
  int rv;
  
  // clearing and setting up hints
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // use my IP
  
  // get what we need to make initial socket clients will connect to
  if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }
  
  // loop through all the results and bind to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("server: socket");
      continue;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("server: bind");
      continue;
    }

    break;
  }
  
  freeaddrinfo(servinfo); // all done with this structure
  
  if (p == NULL) {
    fprintf(stderr, "server: failed to bind\n");
    exit(1);
  }
  
  if (listen(sockfd, BACKLOG) == -1) {
    perror("listen");
    exit(1);
  }
  
  printf("server: waiting for connections...\n");
  
  // main accept() loop
  while(1) {
    sin_size = sizeof their_addr;
    int* new_fd_ptr = (int*)malloc(sizeof(int)); // to avoid race condition if we make all threads just use the same new_fd variable
    
    *new_fd_ptr = accept(sockfd, (struct sockaddr*)&their_addr, &sin_size);
    
    // get readable IP address of client
    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr*)&their_addr), s, sizeof s);
    printf("server: got connection from %s\n", s);
    
    if(*new_fd_ptr == -1){
      perror("accept");
      free(new_fd_ptr);
      continue;
    }
    
    // make thread to handle clients connection
    pthread_t thread_id;
    if(pthread_create(&thread_id, NULL, handle_client, new_fd_ptr) != 0){
      perror("pthread_create");
      free(new_fd_ptr);
      continue;
    }
    
    // detatch thread to prevent memory leaks
    pthread_detach(thread_id);
  }
  
  return 0;
  
}
