#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
/*use this tcp server to handle your multiple request from your mail servers
 * use port 8080 for now*/

#define PORT 8080
#define BUFFER_SIZE 1024

typedef struct {
  pthread_mutex_t lock;
  int connection_no; //no of threads or connections
}connection_state;



int server_setup(struct sockaddr_in * address){
  int server_fd =  socket(AF_INET, SOCK_STREAM, 0);
  int opt = 1 ;

  if (server_fd == -1){
    perror("socket_failed\n");
    exit(EXIT_FAILURE);
  }

  if((setsockopt(server_fd, IPPROTO_TCP, TCP_NODELAY,&opt, sizeof(opt)))!=0)
    printf("failed to set socket options\n");
  if((setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,&opt, sizeof(opt)))!=0)
    printf("failed to set socket options\n");
  

  address->sin_family = AF_INET;
  address->sin_addr.s_addr = INADDR_ANY;
  address->sin_port = htons(PORT);
  
  if((bind(server_fd, (struct sockaddr *)address, sizeof(*address))) != 0){
    perror("Socket binding failed please.\n");
    exit(0);
  }
  return server_fd;
}

void * handle_client(void *connfd){
  /*use this start routine for each connections*/
  int sock = *(int *)connfd;
  char buffer[BUFFER_SIZE];
  int n = read(sock, buffer, BUFFER_SIZE); 
  if( n <= 0 ){
    printf("nothing typed");
  }else{
    buffer[n]  ='\0';
    printf("Client says: %s", buffer);
  }
  char * msg = "Got it babyyyyy!!!";
  send(sock,msg,strlen(msg)+1,0);
  close(sock);
  free(connfd);
  return NULL;
}


void  server_listen_and_respond(int server_fd,struct sockaddr_in* address){
  /* listen and respond to multiple clients 
   * using threads for each  connection. */
  socklen_t addrlen = sizeof(*address);
  if (listen(server_fd, 5) == -1) {
    perror("failed to listen");
    exit(0);
  }
  printf("Server is listening on port %d \n",PORT);

  while(1){
    int new_socket  = accept(server_fd,(struct sockaddr *)address,&addrlen);
    if(new_socket == -1){
      perror("connection failed");
      continue;
    }
    int *new_sock = (int *) malloc(sizeof(int));
    *new_sock  = new_socket;

    pthread_t thread_id;

    pthread_create(&thread_id, NULL, handle_client, (void *)new_sock);
    pthread_detach(thread_id);
  }
    
  close(server_fd);
  
}



//driver code should be removed
int main(int argc, char * argv[]){
  struct sockaddr_in addr;
  int server = server_setup(&addr);
  server_listen_and_respond(server,&addr);

  return 0;
}
