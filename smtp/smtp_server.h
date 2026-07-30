#ifndef SMTP_H
#define SMTP_H
#include "../common/net.h"

#define SMTP_PORT 2525
#define READY 220
#define OK 250
#define MAX_RECIPIENTS 100
#define MAX_MESSAGES 50
#define DATA_MAX (1024 * 1024)  
#define DONE 221

typedef struct{
  char f_msg[128];
}sv_response;

typedef enum{
  SMTP_INIT,     //waiting for HELO/EHLO
  SMTP_GREETED,   //waiting for MAIL FROM 
  SMTP_HAVE_FROM, //waiting for RCPT TO (can  repeat  for  multiple recipients)
  SMTP_HAVE_RCPT, //waiting  for DATA or another RCPT TO 
  SMTP_IN_DATA  //accumulating  message body until lone "."
}smtp_state;

typedef struct {
  sv_response  responses;
  smtp_state status;
  char mail_from[128];
  char  messages[DATA_MAX];
  size_t data_len;
  int recipient_count;
  int message_count;
  char  mail_to[MAX_RECIPIENTS][128];
}smtp_session;

//helper functions.
int extract_email(char *, char * , size_t);
int send_ok(int sock, sv_response * response);
void gen_server_message(sv_response *,int ,const char*);
int handle_recipients(int sock,smtp_session * session_state,char cl_response[],char command[],char rest[]);
int check_envelope_commands(const char * command1,const char * command2,char cl_response[], char  command[],char rest[],int delinumber);
void cleanup(int , client_info*);
void clear_state(smtp_session *);


int greeting(int , smtp_session *);
int check_envelopes(int, smtp_session *);
int get_messages(int, smtp_session *);

void * smtp_handle_client(void * conninfo); 

#endif
