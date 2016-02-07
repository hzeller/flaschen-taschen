// -*- mode: cc; c-basic-offset: 4; indent-tabs-mode: nil; -*-
// Various servers we support, all talking to the same display.

#ifndef FT_SERVER_H
#define FT_SERVER_H

#include "flaschen-taschen.h"

void opc_server_init(int port);
void opc_server_run(FlaschenTaschen *display);

#endif // OPC_SERVER_H

