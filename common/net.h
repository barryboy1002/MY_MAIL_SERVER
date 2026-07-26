#ifndef NET_H
#define NET_H
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#define BUFFER_SIZE 1024

typedef struct {
  pthread_mutex_t lock;
  int connection_no; //no of threads or connections
}connection_state;

typedef struct{
  /*to track a clients state*/
  int  conn_fd;
  connection_state * c_state;
}client_info;

typedef void*(*client_handler_fn)(void *);

int server_setup(int port ,struct sockaddr_in * address);
void * handle_client(void *conninfo);
void  server_listen_and_respond(int server_fd, struct sockaddr_in* address, int port, client_handler_fn handler);

#endif 
