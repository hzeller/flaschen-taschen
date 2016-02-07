// -*- mode: cc; c-basic-offset: 4; indent-tabs-mode: nil; -*-
// quick hack.
// strip data output on stdout, so use in a pipe with socat
// ./send-image foo.ppm | socat STDIO /dev/ttyUSB1,raw,echo=0,crtscts=0,b115200

// BSD-source for usleep()
#define _BSD_SOURCE

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <vector>

#include "flaschen-taschen.h"

#define FLASCHEN_TASCHEN_WIDTH 10

// Read line, skip comments.
char *ReadLine(FILE *f, char *buffer, size_t len) {
    char *result;
    do {
        result = fgets(buffer, len, f);
    } while (result != NULL && result[0] == '#');
    return result;
}

// Load PPM and return strip-data as one-dimensional vector (right now, we expect
// this to be exactly height=5 like in our test-setup).
std::vector<Color> *LoadPPM(FILE *f, int expected_height) {
#define EXIT_WITH_MSG(m) { fprintf(stderr, "%s: |%s", m, line); \
        fclose(f); return NULL; }

    if (f == NULL)
        return NULL;
    char header_buf[256];
    // Header contains widht and height.
    const char *line = ReadLine(f, header_buf, sizeof(header_buf));
    if (sscanf(line, "P6 ") == EOF)
        EXIT_WITH_MSG("Can only handle P6 as PPM type.");
    line = ReadLine(f, header_buf, sizeof(header_buf));
    int width, height;
    if (!line || sscanf(line, "%d %d ", &width, &height) != 2)
        EXIT_WITH_MSG("Width/height expected");
    int value;
    line = ReadLine(f, header_buf, sizeof(header_buf));
    if (!line || sscanf(line, "%d ", &value) != 1 || value != 255)
        EXIT_WITH_MSG("Only 255 for maxval allowed.");

    // Hardcoded pixel mapping
    if (height != expected_height) {
        EXIT_WITH_MSG("Must match expected height");
    }

    std::vector<Color> *result = new std::vector<Color>();
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Color c;
            if (fread(&c, 3, 1, f) != 1) {
                EXIT_WITH_MSG("was lazy using fread(), and this happened :)");
            }
            result->push_back(c);
        }
    }
#undef EXIT_WITH_MSG
    return result;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {  // Limit to one picture right now.
        fprintf(stderr, "Expected picture.\n");
        return 1;
    }

    // TODO(hzeller): remove hardcodedness.
    WS2811FlaschenTaschen display(10, 5);

    const int images = argc - 1;
    std::vector<Color> *image;
    // Read images, store strip compatible data.
    for (int i = 0; i < images; ++i) {
        FILE *f = fopen(argv[i + 1], "r");
        if (!(image = LoadPPM(f, display.height()))) {
            fprintf(stderr, "Too bad, couldn't read everything\n");
            return 1;
        }
        fclose(f);
    }
    const int image_width = image->size() / display.height();
    fprintf(stderr, "Got image (width=%d)\n", image_width);

    const int direction = 1;
    const int sleep_ms = 200;
    int scroll_start = 0;

    for (;;) {
        // Copy a part of our larger image, starting at scroll_start x-pos
        // into our display sized window.
        for (int x = 0; x < display.width(); ++x) {
            for (int y = 0; y < display.height(); ++y) {
                // The x-position in our image changes while we scroll.
                const int img_x = (scroll_start + image_width + x) % image_width;
                const Color &pixel_color = (*image)[img_x + y * image_width];
                // Our display is upside down. Lets mirror.
                display.SetPixel(x, y, pixel_color);
            }
        }

        display.Send();  // ... and show it.

        if (image_width == display.width()) {
            // Image fits. No scrolling needed.
            return 0;
        }
        usleep(sleep_ms * 1000);
        scroll_start = (scroll_start + direction) % image_width;
    }

    return 0;
}
