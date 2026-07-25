#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#ifndef NET_H
#define NET_H
#define PORT 8080
#define BUFFER_SIZE 1024

typedef struct {
  pthread_mutex_t lock;
  int connection_no; //no of threads or connections
}connection_state;

typedef struct{
  /*to track a clients state*/
  int * connfd;
  connection_state * c_state;
}client_info;
int server_setup(struct sockaddr_in * address);
void * handle_client(void *conninfo);
void  server_listen_and_respond(int server_fd,struct sockaddr_in* address);

#endif // !DEBUG
