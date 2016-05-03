// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// Simple example how to write a client.
// This sets two points. A red at (0,0); a blue dot at (5,5)
//
// By default, connects to the installation at Noisebridge. If using a
// different display (e.g. a local terminal display)
// pass the hostname as parameter:
//
//  ./simple-example localhost
//
// .. or set the environment variable FT_DISPLAY to not worry about it
//
//  export FT_DISPLAY=localhost
//  ./simple-example

#include "udp-flaschen-taschen.h"

#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    const char *hostname = NULL;   // Will use default if not set otherwise.
    if (argc > 1) {
        hostname = argv[1];        // Hostname can be supplied as first arg
    }

    // Open socket and create our canvas.
    const int socket = OpenFlaschenTaschenSocket(hostname);
    UDPFlaschenTaschen canvas(socket, 45, 35);
    canvas.SetOffset(0, 0, 5);

    for (;;) {
        canvas.Fill(Color(255, 255, 255));
        canvas.Send();
        sleep(30);

        canvas.Fill(Color(255, 0, 0));
        canvas.Send();
        sleep(10);

        canvas.Fill(Color(0, 255, 0));
        canvas.Send();
        sleep(10);

        canvas.Fill(Color(0, 0, 255));
        canvas.Send();
        sleep(10);
    }
}
