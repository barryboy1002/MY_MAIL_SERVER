#ifndef SMTP_H
#define SMTP_H
#include "../common/net.h"

#define SMTP_PORT 2525
#define READY 220
#define OK 250



typedef struct{
  char f_msg[128];
}sv_response;

void gen_server_message(sv_response *,int ,const char*);
void cleanup(int , client_info*);
int greeting(int , sv_response *);

void * smtp_handle_client(void * conninfo); 

#endif
