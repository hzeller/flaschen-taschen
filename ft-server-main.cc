// -*- mode: cc; c-basic-offset: 4; indent-tabs-mode: nil; -*-
// Flaschen Taschen Server

#include "opc-server.h"

#define LPD_STRIP_GPIO 11

int main(int argc, const char *argv[]) {
    // TODO(hzeller): remove hardcodedness, provide flags
    WS2811FlaschenTaschen top_display(10, 5);
    LPD6803FlaschenTaschen bottom_display(LPD_STRIP_GPIO, 10, 5);
    StackedFlaschenTaschen display(&top_display, &bottom_display);
    display.Send();  // Initialize.

    run_opc_server(7890, &display);
}
