#include "net.h"
/*use this tcp server to handle your multiple request from your mail servers
 * use port 8080 for now*/


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

void * handle_client(void *conninfo){
  /*use this start routine for each connections*/
  client_info * c_info = (client_info*)conninfo; 
  int sock = c_info->conn_fd;
  char buffer[BUFFER_SIZE];
  int n = read(sock, buffer, BUFFER_SIZE);
  pthread_mutex_lock(&(c_info->c_state->lock));
    c_info->c_state->connection_no++;
    fprintf(stderr, "active connections: %d\n", c_info->c_state->connection_no);
  pthread_mutex_unlock(&(c_info->c_state->lock));
  if( n <= 0 ){
    fprintf(stderr, "nothing typed\n");
  }else{
    buffer[n]  ='\0';
    printf("Client says: %s", buffer);
  }
  char * msg = "Got it babyyyyy!!!";
  send(sock,msg,strlen(msg)+1,0);
  close(sock);
   pthread_mutex_lock(&(c_info->c_state->lock));
    c_info->c_state->connection_no--;
    fprintf(stderr, "active connections: %d\n", c_info->c_state->connection_no);
  pthread_mutex_unlock(&(c_info->c_state->lock));
  free(c_info);
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
  
  connection_state cs ={.lock = PTHREAD_MUTEX_INITIALIZER,
                        .connection_no  =0};
  while(1){
    int new_socket  = accept(server_fd,(struct sockaddr *)address,&addrlen);
    if(new_socket == -1){
      perror("connection failed");
      continue;
    }
    client_info *  c_info = (client_info *)malloc(sizeof(client_info));
    c_info->conn_fd = new_socket;
    c_info->c_state = &cs;

    pthread_t thread_id;
    
    pthread_create(&thread_id, NULL, handle_client, (void *)c_info);
    pthread_detach(thread_id);
  }
    
  close(server_fd);
  
}

//driver code 
int main(int argc, char * argv[]){
  struct sockaddr_in addr;
  int server_fd = server_setup(&addr);
  server_listen_and_respond(server_fd,&addr);

  return 0;

}



