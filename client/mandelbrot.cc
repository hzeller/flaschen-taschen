// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// Mandelbrot set.
//
// If you run at Noisebridge, call the binary with the Noisebridge
// display hostname flaschen-taschen.local:
//
// ./mandelbrot flaschen-taschen.local

#include "udp-flaschen-taschen.h"

#include <stdio.h>
#include <strings.h>

#define DISPLAY_WIDTH  45
#define DISPLAY_HEIGHT 35

// Straight from Wikipedia.
void mandelbrot(FlaschenTaschen *display) {
    const int max_iter = 160;

    const float xmin = -2;
    const float xmax = 0.8;
    const float ymin = -1.4;
    const float ymax = 1.4;

    for (int yscreen = 0; yscreen < DISPLAY_HEIGHT; ++yscreen) {
        const float y0 = ymin + ((ymax - ymin) / DISPLAY_HEIGHT) * yscreen;
        for (int xscreen = 0; xscreen < DISPLAY_WIDTH; ++xscreen) {
            const float x0 = xmin + ((xmax - xmin) / DISPLAY_WIDTH) * xscreen;
            float x = 0;
            float y = 0;
            uint8_t iteration = 0;
            while (x*x + y*y < 4 && iteration < max_iter) {
                float xtmp = x*x - y*y + x0;
                y = 2*x*y + y0;
                x = xtmp;
                ++iteration;
            }
            Color c;
            // Some rainbow scaling. TODO: better coloring scheme.
            const int half_way = max_iter/2;
            const float color_scale = 255.0 / half_way;
            c.r = iteration < half_way ? 255 - iteration * color_scale : 0;
            c.g = iteration < half_way
                              ? (iteration * color_scale)
                              : (255 - (iteration - half_way) * color_scale);
            c.b = iteration > half_way ? (iteration - half_way) * color_scale : 0;
            display->SetPixel(xscreen, yscreen, c);
        }
    }
}

int main(int argc, char *argv[]) {
    const char *hostname = "127.0.0.1";
    if (argc > 1) {
        hostname = argv[1];     // Single command line argument.
    }
    fprintf(stderr, "Sending to %s\n", hostname);

    // Open socket and create our canvas.
    const int socket = OpenFlaschenTaschenSocket(hostname);
    UDPFlaschenTaschen canvas(socket, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    mandelbrot(&canvas);
    canvas.Send();
}
