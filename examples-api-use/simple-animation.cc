// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// Simple example how to write an animation. Prepares two UDPFlaschenTaschen
// with one frame of an invader animation each and shows them in sequence
// while modifying the position on the screen.
//
// By default, connects to the installation at Noisebridge. If using a
// different display (e.g. a local terminal display)
// pass the hostname as parameter:
//
//  ./simple-animation localhost
//
// .. or set the environment variable FT_DISPLAY to not worry about it
//
//  export FT_DISPLAY=localhost
//  ./simple-animation

#include "udp-flaschen-taschen.h"

#include <vector>
#include <unistd.h>
#include <stdio.h>

// Size of the display.
#define DISPLAY_WIDTH 45
#define DISPLAY_HEIGHT 35

// The 'layer' we're showing the space-invaders in. The background layer
// in a FlaschenTaschen display is 0. With 1, we're hovering above that.
#define Z_LAYER 7

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
UDPFlaschenTaschen *CreateFromPattern(int socket, const char *invader[],
                                      const Color &color) {
    // We leave a frame of one pixel around the sprite, so that if we move
    // them around by one pixel, previous pixels are overwritten with black.
    UDPFlaschenTaschen *result = new UDPFlaschenTaschen(socket,
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
    const char *hostname = NULL;   // Will use default if not set otherwise.
    if (argc > 1) {
        hostname = argv[1];        // Hostname can be supplied as first arg
    }

    // Open socket.
    const int socket = OpenFlaschenTaschenSocket(hostname);

    // Prepare a couple of frames ready to send.
    std::vector<UDPFlaschenTaschen*> frames;
    frames.push_back(CreateFromPattern(socket, invader[0], Color(255, 255, 0)));
    frames.push_back(CreateFromPattern(socket, invader[1], Color(255, 0, 255)));

    const int max_animation_x = 45;
    const int max_animation_y = 35;
    int animation_x = 0;
    int animation_y = 0;
    int animation_direction = +1;

    for (unsigned i = 0; /**/; ++i) {
        UDPFlaschenTaschen *current_frame = frames[i % frames.size()];

        // Our animation offset determines where on the FlaschenTaschen
        // display our frame will be displayed.
        // We use the z-layering here to hover above the background.
        current_frame->SetOffset(animation_x, animation_y, Z_LAYER);

        current_frame->Send();      // Send the framebuffer.
        usleep(300 * 1000);         // wait until we show next frame.

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
