// -*- mode: cc; c-basic-offset: 4; indent-tabs-mode: nil; -*-
// Various servers we support, all talking to the same display.

#ifndef FT_SERVER_H
#define FT_SERVER_H

#include "flaschen-taschen.h"
#include <pthread.h>

bool opc_server_init(int port);
void opc_server_run(FlaschenTaschen *display, pthread_mutex_t *mutex);

bool udp_server_init(int port);
void udp_server_run(FlaschenTaschen *display, pthread_mutex_t *mutex);

#endif // OPC_SERVER_H

