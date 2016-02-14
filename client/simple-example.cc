// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// Simple example how to write a client.
// This sets two points. A red at (0,0); a blue dot at (5,5)

#include "udp-flaschen-taschen.h"

#define DISPLAY_WIDTH  20
#define DISPLAY_HEIGHT 20

int main() {
    // Open socket and create our canvas.
    const int socket = OpenFlaschenTaschenSocket("flaschen-taschen.local");
    UDPFlaschenTaschen canvas(socket, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    const Color red(255, 0, 0);
    canvas.SetPixel(0, 0, red);              // Sample with color variable.
    canvas.SetPixel(5, 5, Color(0, 0, 255)); // or just use inline (here: blue).

    canvas.Send();                           // Send the framebuffer.
}
