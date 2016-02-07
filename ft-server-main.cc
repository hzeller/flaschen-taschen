// -*- mode: cc; c-basic-offset: 4; indent-tabs-mode: nil; -*-

#include "opc-server.h"

int main(int argc, const char *argv[]) {
    // TODO(hzeller): remove hardcodedness, provide flags
    WS2811FlaschenTaschen display(10, 5);
    
    run_opc_server(7890, &display);
}
