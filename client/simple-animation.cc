// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// Simple example how to write a client.
// This sets two points. A red at (0,0); a blue dot at (5,5)
// If you run your local server (e.g. the terminal server), just call binary
// with 127.0.0.1 instead of flaschen-taschen.local
//
//  ./simple-animation 127.0.0.1

#include "udp-flaschen-taschen.h"

#include <vector>
#include <unistd.h>

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
    const char *hostname = "flaschen-taschen.local";
    if (argc > 1) {
        hostname = argv[1];     // Single command line argument.
    }

    // Open socket.
    const int socket = OpenFlaschenTaschenSocket(hostname);

    // Prepare a couple of frames ready to send.
    std::vector<UDPFlaschenTaschen*> frames;
    frames.push_back(CreateFromPattern(invader[0], Color(255, 255, 0)));
    frames.push_back(CreateFromPattern(invader[1], Color(255, 0, 0)));

    const int max_animation_x = 20;
    const int max_animation_y = 20;
    int animation_x = 0;
    int animation_y = 0;
    int animation_direction = +1;

    for (unsigned i = 0; /**/; ++i) {
        UDPFlaschenTaschen *current_frame = frames[i % frames.size()];

        // Our animation offset determines where on the FlaschenTaschen
        // display our frame will be displayed.
        current_frame->SetOffset(animation_x, animation_y);

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
