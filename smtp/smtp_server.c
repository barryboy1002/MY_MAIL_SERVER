#include "smtp_server.h"

void cleanup(int sock,client_info * c_info){
  close(sock);
  free(c_info);
}

void gen_server_message(sv_response * response,int code, const char * msg ){
  snprintf(response->f_msg,sizeof(response->f_msg),"%d %s\r\n",code,msg);
}

int send_ok(int sock, sv_response * response){
  const char * init_msg2 = " OK";
  gen_server_message(response,OK,init_msg2);
  if((send(sock,response->f_msg , strlen(response->f_msg),0))== -1){
    perror("send()");
    return -1;
  }
  return 0;
}



int greeting(int sock, smtp_session * session_state){
  const char * init_msg = "mail.example.com";
  gen_server_message(&(session_state->responses),READY,init_msg);
  
  if((send(sock, session_state->responses.f_msg ,strlen(session_state->responses.f_msg),0))== -1){
    perror("send()");
    return -1;
  }
  
  session_state->status = SMTP_INIT;
  
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
  
  int ok = send_ok(sock, &(session_state->responses));
  if(ok < 0){
    return -1;
  }
  session_state->status = SMTP_GREETED;
  return 0;
}


int check_envelopes(int sock ,smtp_session * session_state){
  char cl_response[128];
  int rc = read_line(sock,cl_response,128);
  if( rc <= 0){
    fprintf(stderr, "failed to read %d\n", rc);
    return -1;
  }

  //processing the MAIL FROM
  char command[16] = {0};
  char rest[112] = {0};
  sscanf(cl_response,"%15s %111s", command,rest);
  command[15] = '\0'; 
  rest[111] = '\0';
  if((strcasecmp(command,"MAIL")) != 0){
    fprintf(stderr,"Invalid command %s\n", command);
    return -1;
  }
  if((strncasecmp(rest,"FROM:",5)) != 0){
    fprintf(stderr,"Invalid command %s\n", command);
    return -1;
  }
  
  //will add storage and stuff but for now jus 
  int ok = send_ok(sock, &(session_state->responses));
  if(ok < 0){
    return -1;
  }
  session_state->status = SMTP_HAVE_FROM;

  return 0;


}

void * smtp_handle_client(void * conninfo){
  client_info *  c_info  = (client_info * )conninfo;
  int sock = c_info->conn_fd;
  
  
  smtp_session  session_state;

  if(greeting(sock,&session_state) == -1){
    cleanup(sock,c_info);
    return NULL;
  }
  if (session_state.status == SMTP_GREETED){
    int rc = check_envelopes(sock,&session_state);
    if ( rc == -1){
      cleanup(sock,c_info);
      return NULL;
    }
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
