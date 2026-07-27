#include "smtp_server.h"

void cleanup(int sock,client_info * c_info){
  close(sock);
  free(c_info);
}

int greeting(int sock){
}


void * smtp_handle_client(void * conninfo){
  client_info *  c_info  = (client_info * )conninfo;
  int sock = c_info->conn_fd;

  if(greeting(sock) == -1){
    cleanup(sock,c_info);
    return NULL;
  }

  cleanup(sock,c_info);
  return NULL;
}

int main(int argc, char *argv[]){
  struct sockaddr_in addr;
  int server_fd = server_setup(SMTP_PORT, &addr);
  server_listen_and_respond(server_fd,&addr,SMTP_PORT,smtp_handle_client);

  return 0;
}
