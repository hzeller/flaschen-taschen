// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// Simple example how to write an animation. Prepares two UDPFlaschenTaschen
// with one frame of an invader animation each and shows them in sequence
// while modifying the position on the screen.
//
// If you run at Noisebridge, call the binary with the Noisebridge
// display hostname flaschen-taschen.local:
//
// ./simple-animation flaschen-taschen.local

#include "udp-flaschen-taschen.h"

#include <vector>
#include <unistd.h>
#include <stdio.h>

#define INVADER_ROWS 8
#define INVADER_WIDTH 11

static const char* invader[][INVADER_ROWS] = {
    {
        "  #     #  ",   // Exercise: how if different characters
        "   #   #   ",   // would be interpreted as different colors in
        "  #######  ",   // CreateFromPattern() ?
        " ## ### ## ",
        "###########",
        "# ####### #",
        "# #     # #",
        "  ##   ##  ",
    },
    {
        "  #     #  ",
        "#  #   #  #",
        "# ####### #",
        "### ### ###",
        " ######### ",
        "  #######  ",
        "  #     #  ",
        " #       #",
        },
};

// Fill invader from pattern above.
UDPFlaschenTaschen *CreateFromPattern(const char *invader[],
                                      const Color &color) {
    // We leave a frame of one pixel around the sprite, so that if we move
    // them around by one pixel, previous pixels are overwritten with black.
    UDPFlaschenTaschen *result = new UDPFlaschenTaschen(-1,
                                                        INVADER_WIDTH + 2,
                                                        INVADER_ROWS + 2);
    for (int row = 0; row < INVADER_ROWS; ++row) {
        const char *line = invader[row];
        for (int x = 0; line[x]; ++x) {
            if (line[x] != ' ') {
                result->SetPixel(x + 1, row + 1, color);
            }
        }
    }
    return result;
}

int main(int argc, char *argv[]) {
    const char *hostname = "127.0.0.1";
    if (argc > 1) {
        hostname = argv[1];     // Single command line argument.
    }
    fprintf(stderr, "Sending to %s\n", hostname);

    // Open socket.
    const int socket = OpenFlaschenTaschenSocket(hostname);

    // Prepare a couple of frames ready to send.
    std::vector<UDPFlaschenTaschen*> frames;
    frames.push_back(CreateFromPattern(invader[0], Color(255, 255, 0)));
    frames.push_back(CreateFromPattern(invader[1], Color(255, 0, 255)));

    const int max_animation_x = 20;
    const int max_animation_y = 20;
    int animation_x = 0;
    int animation_y = 0;
    int animation_direction = +1;

    for (unsigned i = 0; /**/; ++i) {
        UDPFlaschenTaschen *current_frame = frames[i % frames.size()];

        // Our animation offset determines where on the FlaschenTaschen
        // display our frame will be displayed.
        // We use the z-layering here: we are always one above the background.
        current_frame->SetOffset(animation_x, animation_y, 1);

        current_frame->Send(socket);      // Send the framebuffer.
        usleep(300 * 1000);               // wait until we show next frame.

        // Update position of space invader.
        if (i % 2 == 0) {
            animation_x += animation_direction;
            if (animation_x > max_animation_x) {
                animation_direction = -1;
                animation_y += 1;
            }
            if (animation_x < 1) {
                animation_direction = +1;
                animation_y += 1;
            }
            if (animation_y >= max_animation_y) {
                // Here, we jump, showing how the last position is not cleared.
                animation_y = 0;
            }
        }
    }
}
