// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// Simple example how to write a client.

#include "udp-flaschen-taschen.h"

#define DISPLAY_WIDTH 10
#define DISPLAY_HEIGHT 10

int main() {
    int socket = OpenFlaschenTaschenSocket("flaschen-taschen.local");
    UDPFlaschenTaschen canvas(socket, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    Color red(255, 0, 0);
    Color blue(0, 0, 255);
    canvas.SetPixel(0, 0, red);
    canvas.SetPixel(5, 5, blue);

    canvas.Send();
}
