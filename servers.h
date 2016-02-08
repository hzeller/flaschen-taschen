// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
// Various servers we support, all talking to the same display.

#ifndef FT_SERVER_H
#define FT_SERVER_H

#include "flaschen-taschen.h"
#include "thread.h"

bool opc_server_init(int port);
void opc_server_run_blocking(FlaschenTaschen *display, ft::Mutex *mutex);

bool udp_server_init(int port);
void udp_server_run_blocking(FlaschenTaschen *display, ft::Mutex *mutex);

bool pixel_pusher_init(FlaschenTaschen *canvas);
void pixel_pusher_run_threads(FlaschenTaschen *display, ft::Mutex *mutex);

#endif // OPC_SERVER_H

