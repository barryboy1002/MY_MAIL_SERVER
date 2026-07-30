#include "smtp_server.h"

//Starting ar some helper functions
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


int handle_recipients(int sock,smtp_session * session_state,char cl_response[],char command[],char rest[]){
  while(1){
    int rc = read_line(sock,cl_response,128);
    if( rc <= 0){
      fprintf(stderr,"failed to read %d\n",rc);
      return -1;
    }
    if(check_envelope_commands("RCPT", "TO:", cl_response,command,rest,3) == 0){
      if(session_state->recipient_count >= MAX_RECIPIENTS){
        fprintf(stderr,"too many recipients\n");
        return -1;
      }
      char * dest = session_state->mail_to[session_state->recipient_count];
      if(extract_email(rest,dest,sizeof(session_state->mail_to[0]))==-1) {
        fprintf(stderr,"bad RCPT TO address\n");
        return -1;
      }
      printf("%s\n",session_state->mail_to[session_state->recipient_count]); 
      session_state->recipient_count ++;
      session_state->status = SMTP_HAVE_RCPT;
      if (send_ok(sock, &session_state->responses) < 0) return -1;
      continue;
    }

    if(strcasecmp(command,"DATA") == 0){
      if(session_state->recipient_count == 0){
        fprintf(stderr,"DATA WITH NO RECIPIENTS\n");
        return -1; //I know real smtp would return 503
      }
      return 0;
    }
    fprintf(stderr, "unexpected command while collecting recipients: %s\n", command);
    return -1;
  }
}


int check_envelope_commands(const char * command1,const char * command2,char cl_response[], char  command[],char rest[],int delinumber){
  sscanf(cl_response,"%15s %111s", command,rest);
  command[15] = '\0'; 
  rest[111] = '\0';
  if((strcasecmp(command,command1)) != 0){
    fprintf(stderr,"Invalid command %s\n", command);
    return -1;
  }

  memset(command,0, strlen(command));
  if((strncasecmp(rest,command2,delinumber)) != 0){
    fprintf(stderr,"Invalid command %s\n", rest);
    return -1;
  }
  return 0;

}

int extract_email(char * field,char * out,size_t outlen){
  const char * start = strchr(field,'<');
  const char * end = strchr(field, '>');

  if(!start||!end ||end <= start) return -1;
  
  size_t len = end - start -1 ;
  if (len >= outlen )return -1 ;

  memcpy(out,start+1,len);
  out[len] = '\0';
  memset(field,0,strlen(field));

  return 0;
}

void clear_state(smtp_session * session_state){
  memset(session_state->mail_from,0,strlen(session_state->mail_from));
  for(int i = 0 ; i < session_state->recipient_count;i++){
    memset(session_state->mail_to[i], 0 , strlen(session_state->mail_to[i]));
  }
  memset(session_state->messages,0,strlen(session_state->messages));
  session_state->recipient_count = 0;
}

/*These are the main procedural functions
 * greeting  to ensure(HELO)
 * check_envelopes  -> ( MAIL FROM) and (RCPT TO)
 * get_email -> DATA     */

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
  if(check_envelope_commands("MAIL", "FROM:", cl_response,command,rest,5) == -1) return -1;
  
  if(extract_email(rest,session_state->mail_from,sizeof(session_state->mail_from))==-1) return -1;
  printf("%s\n",session_state->mail_from); 
  //will add storage and stuff but for now jus 
  int ok = send_ok(sock, &(session_state->responses));
  if(ok < 0){
    return -1;
  }

  session_state->status = SMTP_HAVE_FROM;
  
  memset(cl_response,0, strlen(cl_response));
  
  if(handle_recipients(sock, session_state,cl_response,command,rest) == 0){
    const char * init_msg = "Send message content; end with \r\n,\r\n";
    gen_server_message(&(session_state->responses),READY,init_msg);
    if((send(sock, session_state->responses.f_msg ,strlen(session_state->responses.f_msg),0))== -1){
      perror("send()");
      return -1;
    }
    session_state->status = SMTP_IN_DATA;
  }

 return 0;
}


int get_messages(int sock, smtp_session * session_state){
  session_state->data_len = 0;
  session_state->messages[0] ='\0';
  while(1){
    char data_line[DATA_MAX];
    int rc = read_line(sock,data_line, DATA_MAX);
    if(rc <= 0){
      fprintf(stderr, "failed to read%d\n", rc);
      return -1;
    }
    if(strcmp(data_line,".")==0){
      break;
    }
    size_t linelen = strlen(data_line);
    if(session_state->data_len + linelen + 1 >= sizeof(session_state->messages)){
      fprintf(stderr,"message too large\n");
      return -1;
    }

    memcpy(session_state->messages+session_state->data_len,data_line,linelen);
    session_state->data_len += linelen;
    session_state->messages[session_state->data_len++] ='\n';
    session_state->messages[session_state->data_len] = '\0';


  }
  if (send_ok(sock,&(session_state->responses)) == -1) return -1;
  printf("%s\n",session_state->messages);
  return 0;
}

int reset_connection(int sock, smtp_session * session_state){
  char finalmsg[128];
  int rc = read_line(sock,finalmsg,128);
  if( rc <= 0){
    fprintf(stderr,"Failed to read %d\n", rc);
    return -1;
  }
  
  if(strcasecmp(finalmsg,"QUIT") == 0){
    const char * done_msg = "Bye";
    gen_server_message(&(session_state->responses),DONE,done_msg);
  
    if((send(sock, session_state->responses.f_msg ,strlen(session_state->responses.f_msg),0))== -1){
      perror("send()");
      return -1;
    }
  
    return 0;
  }
  printf("so you want more\n");
  clear_state(session_state);
  session_state->status = SMTP_GREETED;
  return 1;
}


void * smtp_handle_client(void * conninfo){
  client_info *  c_info  = (client_info * )conninfo;
  int sock = c_info->conn_fd;  
  
  smtp_session  session_state;

  if(greeting(sock,&session_state) == -1){
    cleanup(sock,c_info);
    return NULL;
  }
  while(1){
    if (session_state.status == SMTP_GREETED){
      int rc = check_envelopes(sock,&session_state);
      if ( rc == -1){
        break;
      }
    }
    if(session_state.status == SMTP_IN_DATA){
      int rc = get_messages(sock,&session_state);
      if(rc == -1){
        break;
      }
    }
    if(session_state.status == SMTP_IN_DATA){ 
      int rc = reset_connection(sock,& session_state); 
      if((rc== 0)||(rc == -1)){
        break;
      }
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
