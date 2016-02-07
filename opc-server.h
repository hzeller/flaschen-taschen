// -*- mode: cc; c-basic-offset: 4; indent-tabs-mode: nil; -*-
// Various servers we support, all talking to the same display.

#ifndef OPC_SERVER_H
#define OPC_SERVER_H

#include "flaschen-taschen.h"

void run_opc_server(int port, FlaschenTaschen *display);

#endif // OPC_SERVER_H

