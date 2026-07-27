#ifndef SMTP_H
#define SMTP_H
#include "../common/net.h"

#define SMTP_PORT 2525

void cleanup(int , client_info*);
int greeting(int sock);

void * smtp_handle_client(void * conninfo); 

#endif
