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
#include "packet.h"

#define PORT "3456"
#define BACKLOG 10// how many pending connections queue will hold
#define MAXINPUTARGS 5

// to help receive packets
void recv_packet(int sockfd, Packet* pkt){
  char pktBuf[MAXPACKSIZE];
  ssize_t numBytes = 0;
  // clear buffer before using
  memset(pktBuf, 0, MAXPACKSIZE);
  
  // receive the packet
  if((numBytes = recv(sockfd, pktBuf, MAXPACKSIZE - 1, 0)) == -1){
      perror("recv");
      close(sockfd);
      exit(1);
  }
  //printf("Received: %s\n", pktBuf);
  pktBuf[numBytes] = '\0';
  
  // parse packets information
  pkt->deserialize(pkt, pktBuf);
}

// to make turn the filename into an executable
char* rmExtnsn(char* file){
  char *dotPos = strchr(file, '.');
  size_t newLen = (dotPos != NULL) ? (size_t)(dotPos - file) : strlen(file);
  
  // allocate memory for executable
  char* exe = (char*)malloc(newLen + 1);
  
  // copy characters up to .
  strncpy(exe, file, newLen);
  exe[newLen] = '\0';
  
  return exe;
}

// function to compile a file
// IF THIS FUNCTION IS USED RUN ALSO NEEDS TO BE USED
char* compile(char* filename){
  pid_t pid;
  int status;
  char* executable = rmExtnsn(filename);
  
  // make child process to compile
  if(!(pid = fork())){
    char *args[] = {"gcc", filename, "-o", executable, NULL};
    if(execvp(args[0], args)){
      // if execvp returns, an error occured
      perror("execvp");
      exit(EXIT_FAILURE);
    }
  }

  // wait for compilation to complete
  waitpid(pid, &status, 0);
  printf("Compiled %s\n", filename);
  
  // make executable produced runable
  if(chmod(executable, S_IRWXU) == -1){
    perror("chmod failed");
    exit(EXIT_FAILURE);
  }
  
  return executable; // note that this is still allocated on the heap and still needs to be freed
}

void run(char* executable){
  pid_t pid;
  int status;
  char arg0[strlen(executable) + 3];
  
  // making first argument
  snprintf(arg0, sizeof(arg0), "./%s", executable);
  
  // make child process to run executable
  if(!(pid = fork())){
    char *args[] = {arg0, "9", NULL}; // currently just testing with 9
    if(execvp(args[0], args)){ // need to see if we can get 
      // if execvp returns, an error occured
      perror("execvp");
      exit(EXIT_FAILURE);
    }
  }
  
  waitpid(pid, &status, 0);
  printf("Ran %s\n", executable);
  return;
}

// trying a basic server shell the client can send commands to, then, add the file things in
void* new_handle_client(void *arg){
  
  int new_fd = *((int*)arg);
  free(arg); // dealocating memory allocated for clients file descriptor
  char buf[MAXDATASIZE];  
  Packet tempPack;
  bool connection = true;
  bool recFile = false;
  bool testFile = false;
  FILE* fp = NULL;
  char* filename = NULL;
  
  // initializing variables
  tempPack.initialize = initializePacket;
  tempPack.initialize(&tempPack, 0, "");
  
  // clear buffers before using
  memset(buf, 0, MAXDATASIZE);
  
  // loop forever taking commands from client
  // note that the recv function should be a blocking call, so we shouldn't be wasting cycles
  while(connection){
    
    recv_packet(new_fd, &tempPack);
    
    printf("\nreceived: \n%s\n", tempPack.payload);
    
    // decide what to do once the packet has been received
    switch(tempPack.flag){
      case 0:
        printf("Received user input packet\n");
        
        // tokenize input into arguments
        char* token = strtok(tempPack.payload, " ");
        int toki = 0;
        char* inputArgs[MAXINPUTARGS];
        while(token != NULL && toki < MAXINPUTARGS){
          inputArgs[toki] = token;
          token = strtok(NULL, " ");
          toki++;
        }
        inputArgs[toki] = NULL;
        
        if(!strcmp(inputArgs[0], "test")) {
          // we know we are going to be doing a test now
          testFile = true;
          // so we know we should be receiving a file
          recFile = true;
          
          // populating filename with threadId followed by name of file to test
          asprintf(&filename, "%"PRIu64 "%s", (uint64_t)pthread_self(), inputArgs[1]);
          
          // create the file with filename
          fp = fopen(filename, "w");
          
          if (fp == NULL) {
            perror("Error creating file");
            free(filename);
            connection = false;
          }
        }
        break;
      case 1:
        printf("Received file contents packet\n");
        if(recFile){
          fprintf(fp, "%s", tempPack.payload);
        } else {
          printf("Should not have received file contents packet\n");
        }

        break;
      case 3:
        printf("Received EOT packet\n");
        
        if(recFile && fp){
          fclose(fp);
          fp = NULL;
        }
        
        if(testFile) {
          char* executable = compile(filename);
          run(executable);
        }
        
        free(filename);
        filename = NULL;
        recFile = false;
        break;
    }
    
    tempPack.free(&tempPack);
  }
  
  return NULL;
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
    // each thread gets its own new file descriptor pointer
    int* new_fd_ptr = (int*)malloc(sizeof(int)); 
    
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
    if(pthread_create(&thread_id, NULL, new_handle_client, new_fd_ptr) != 0){
      perror("pthread_create");
      free(new_fd_ptr);
      continue;
    }
    
    // detatch thread to prevent memory leaks
    pthread_detach(thread_id);
  }
  
  return 0;
  
}
