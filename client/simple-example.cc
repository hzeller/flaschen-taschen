// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// Simple example how to write a client.
// This sets two points. A red at (0,0); a blue dot at (5,5)
//
// If you run at Noisebridge, call the binary with the Noisebridge
// display hostname flaschen-taschen.local:
//
// ./simple-example flaschen-taschen.local

#include "udp-flaschen-taschen.h"

#include <stdio.h>

#define DISPLAY_WIDTH  20
#define DISPLAY_HEIGHT 20

int main(int argc, char *argv[]) {
    const char *hostname = "127.0.0.1";
    if (argc > 1) {
        hostname = argv[1];     // Single command line argument.
    }
    fprintf(stderr, "Sending to %s\n", hostname);

    // Open socket and create our canvas.
    const int socket = OpenFlaschenTaschenSocket(hostname);
    UDPFlaschenTaschen canvas(socket, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    const Color red(255, 0, 0);
    canvas.SetPixel(0, 0, red);              // Sample with color variable.
    canvas.SetPixel(5, 5, Color(0, 0, 255)); // or just use inline (here: blue).

    canvas.Send();                           // Send the framebuffer.
}
