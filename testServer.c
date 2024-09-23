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
#include "testData.h"

#define PORT "3456"
#define BACKLOG 10// how many pending connections queue will hold
#define MAXINPUTARGS 5
#define PIPEBUFSIZE 100
#define MAXTESTLINELEN 12 // how big each testcase line can be _____,_____

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

// to make sending packets easier
void send_packet(int sockfd, Packet* pkt){
  char* spkt = pkt->serialize(pkt);
  
  printf("\nSending:\n%s\n", spkt);
  
  if(send(sockfd, spkt, strlen(spkt), 0) == -1){
    perror("send");
  }
  free(spkt);
  // so multiple sends streams aren't interpreted as one
  sleep(1);
}

// to remove trailing whitespace and newline characters from expected output
void trim(char* str){
  if(str == NULL) return;
  
  size_t len = strlen(str);
  while(len > 0 && isspace((unsigned char)str[len - 1])) {
    str[len - 1] = '\0';
    len--;
  }
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

// returns the result of the test
/*
  Need to redirect standard output of the code we are running and get it here
*/
char* test(char* executable, char* testcase){
  pid_t pid;
  int status;
  char arg0[strlen(executable) + 3]; // 3 for './' and null terminating character
  int pipefd[2];
  char buf[PIPEBUFSIZE];
  
  // making pipe to get results from child process running executable
  if (pipe(pipefd) == -1){
    perror("pipe");
    exit(EXIT_FAILURE);
  }
  
  // making first argument
  snprintf(arg0, sizeof(arg0), "./%s", executable);
  
  if(!(pid = fork())){ // child process, responsible for running executable
    char *args[] = {arg0, testcase, NULL};
    
    // child not reading
    close(pipefd[0]);
    // redirecting standard output to the pipe
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[1]);
    
    if(execvp(args[0], args)){ 
      // if execvp returns, an error occured
      perror("execvp");
      exit(EXIT_FAILURE);
    }
  } else { // parent process, get results from child process
    // parent not writing
    close(pipefd[1]);
    
    // wait for child to finish running
    waitpid(pid, &status, 0);
    
    // reading output from pipe
    ssize_t numBytes = read(pipefd[0], buf, PIPEBUFSIZE - 1);
    if (numBytes == -1) {
      perror("read");
      exit(EXIT_FAILURE);
    }
    
    buf[numBytes] = '\0';
    // done reading
    close(pipefd[0]);
  }
  
  //printf("Output from %s: \n%s\n", executable, buf);
  return strdup(buf);
}

// populates structure that holds all our tests we will be running
void populateTestCases(TestData* testCases, char* testFile){
  FILE* fp = NULL;
  char line[MAXTESTLINELEN];
  printf("testfile name passed in: %s", testFile);
  fp = fopen(testFile, "r");

  if (fp == NULL) {
    perror("Error opening file");
    exit(EXIT_FAILURE);
  }
  
  // read testcases and store them
  while(fgets(line, sizeof(line), fp)){
    char* input = strtok(line, ",");
    char* expectedOutput = strtok(NULL, ",");
    
    if (input && expectedOutput) {
      // getting rid of trailing whitespace and newlines
      trim(expectedOutput);
    
      testCases->addTestCase(testCases, input, expectedOutput);
    } else {
      perror("Invalid line format");
    }
  }
  
  fclose(fp);
}

// tests the compiled executable using the testcases
char* runTests(char* executable, TestData* testCases) {
  // so results can hold "Passed: ___\nFailed ___\n\0", so we can have up to 999 testcases
  char* results = (char*)malloc(25 * sizeof(char));
  int passed = 0;
  int failed = 0;
  
  for(int i = 0; i < testCases->size; i++) {
    TestCase* tc = testCases->getTestCase(testCases, i);
    char* output = test(executable, tc->input);
    
    // show results on server but change later to let client know
    if(strcmp(output, tc->expectedOutput) == 0) {
      printf("Test %d passed\n", i+1);
      passed++;
    } else {
      printf("Test %d failed: expected %s, got %s\n", i+1, tc->expectedOutput, output);
      failed++;
    }
  }
  // formating results to send to client
  snprintf(results, 25, "Passed: %d\nFailed: %d\n", passed, failed);
  return results;
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
  TestData testCases;

  
  // initializing variables
  tempPack.initialize = initializePacket;
  tempPack.initialize(&tempPack, 0, "");

  testCases.initialize = initializeTestData;
  testCases.initialize(&testCases);
  
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
          printf("input arg: %s", inputArgs[toki]);
          token = strtok(NULL, " ");
          toki++;
        }
        inputArgs[toki] = NULL;
        
        int kounter = 0;
        while(inputArgs[kounter] != NULL){
          printf("input arg %d: %s\n", kounter, inputArgs[kounter]);
          kounter++;
        }
        
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
          
          // populating testcases
          printf("Testcase file name: %s", inputArgs[2]);
          populateTestCases(&testCases, inputArgs[2]);
          
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
        
        if(fp){
          fclose(fp);
          fp = NULL;
        }
        
        if(testFile) {
          char* executable = compile(filename);
          char* results = runTests(executable, &testCases);
          tempPack.initialize(&tempPack, 0, results);
          send_packet(new_fd, &tempPack);
          free(results);
        }
        
        // resetting variables so more testing can be done
        testFile = false;
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
