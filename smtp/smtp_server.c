#include "smtp_server.h"

void cleanup(int sock,client_info * c_info){
  close(sock);
  free(c_info);
}

void gen_server_message(sv_response * response,int code, const char * msg ){
  snprintf(response->f_msg,sizeof(response->f_msg),"%d %s\r\n",code,msg);
}


int greeting(int sock, sv_response * response){
  const char * init_msg = " mail.example.com\n";
  gen_server_message(response,READY,init_msg);

  if((send(sock, response->f_msg ,strlen(response->f_msg),0))== -1){
    perror("send()");
    return -1;
  }
  char cl_response[128];
  int rc = read_line(sock,cl_response,128);
  if( rc <= 0 ){
    fprintf(stderr,"failed to read %d\n",rc);
    return -1;
  }
  
  char  command[16];
  sscanf(cl_response,"%15s", command);
  command[15] ='\0'; 
  if(strcasecmp("HELO",command) != 0) {
    fprintf(stderr,"Invalid command %s\n", command);
    return -1;
  }
  
  const char * init_msg2 = " OK\n";
  gen_server_message(response,OK,init_msg2);
  if((send(sock,response->f_msg , strlen(response->f_msg),0))== -1){
    perror("send()");
    return -1;
  }  
  return 0;
}


void * smtp_handle_client(void * conninfo){
  client_info *  c_info  = (client_info * )conninfo;
  int sock = c_info->conn_fd;
  
  sv_response  responses;
  if(greeting(sock,&responses) == -1){
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
