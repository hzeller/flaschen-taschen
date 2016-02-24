// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
// Copyright (C) 2016 Henner Zeller <h.zeller@acm.org>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation version 2.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://gnu.org/licenses/gpl-2.0.txt>

// To use this image viewer, first get image-magick development files
// $ sudo aptitude install libgraphicsmagick++-dev

#include <assert.h>
#include <math.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <Magick++.h>
#include <magick/image.h>
#include <vector>

#include "udp-flaschen-taschen.h"

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

namespace {
::Color ConvertColor(const Magick::Color &c) {
    return ::Color(ScaleQuantumToChar(c.redQuantum()),
                   ScaleQuantumToChar(c.greenQuantum()),
                   ScaleQuantumToChar(c.blueQuantum()));
}

void CopyImage(const Magick::Image &img, FlaschenTaschen *result) {
    for (size_t y = 0; y < img.rows(); ++y) {
        for (size_t x = 0; x < img.columns(); ++x) {
            const Magick::Color &c = img.pixelColor(x, y);
            if (c.alphaQuantum() >= 255)
                continue;
            result->SetPixel(x, y, ConvertColor(c));
        }
    }
}

// Frame already prepared as the buffer to be sent over so that animation
// and re-sizing operations don't have to be done online. Also knows about the
// animation delay.
class PreprocessedFrame {
public:
    PreprocessedFrame(const Magick::Image &img,
                      int width, int height,
                      int offset_x, int offset_y, int offset_z)
        : width_(width), content_(-1, width, height) {
        assert(width >= (int) img.columns() && height >= (int) img.rows());
        content_.SetOffset(offset_x, offset_y, offset_z);
        int delay_time = img.animationDelay();  // in 1/100s of a second.
        if (delay_time < 1) delay_time = 1;
        delay_micros_ = delay_time * 10000;
        CopyImage(img, &content_);
    }
 
    int width() const { return width_; }

    void Send(int fd) { content_.Send(fd); }

    int delay_micros() const {
        return delay_micros_;
    }

private:
    const int width_;
    UDPFlaschenTaschen content_;
    int delay_micros_;
};
}  // end anonymous namespace

// Load still image or animation.
// Scale, so that it fits in "width" and "height" and store in "result".
static bool LoadImageAndScale(const char *filename,
                              int target_width, int target_height,
                              bool fit_height,
                              std::vector<Magick::Image> *result) {
    std::vector<Magick::Image> frames;
    fprintf(stderr, "Read image...\n");
    readImages(&frames, filename);
    if (frames.size() == 0) {
        fprintf(stderr, "No image found.");
        return false;
    }

    // Put together the animation from single frames. GIFs can have nasty
    // disposal modes, but they are handled nicely by coalesceImages()
    if (frames.size() > 1) {
        fprintf(stderr, "Assembling animation with %d frames.\n",
                (int)frames.size());
        Magick::coalesceImages(result, frames.begin(), frames.end());
    } else {
        result->push_back(frames[0]);   // just a single still image.
    }

    const int img_width = (int)(*result)[0].columns();
    const int img_height = (int)(*result)[0].rows();
    if (fit_height) {
        // In case of scrolling, we only want to scale down to height
        target_width = (int) (1.0*img_width * target_height / img_height);
    }
    fprintf(stderr, "Scale ... %dx%d -> %dx%d\n",
            img_width, img_height, target_width, target_height);
    for (size_t i = 0; i < result->size(); ++i) {
        (*result)[i].scale(Magick::Geometry(target_width, target_height));
    }

    return true;
}

void DisplayAnimation(const std::vector<Magick::Image> &image_sequence,
                      int display_width, int display_height,
                      int offset_x, int offset_y, int offset_z,
                      int fd) {
    std::vector<PreprocessedFrame*> frames;
    // Convert to preprocessed frames.
    for (size_t i = 0; i < image_sequence.size(); ++i) {
        frames.push_back(new PreprocessedFrame(image_sequence[i],
                                               display_width, display_height,
                                               offset_x, offset_y, offset_z));
    }

    for (unsigned int i = 0; !interrupt_received; ++i) {
        if (i == frames.size()) i = 0;
        PreprocessedFrame *frame = frames[i];
        frame->Send(fd);  // Simple. just send it.
        if (frames.size() == 1) {
            return;  // We are done.
        } else {
            usleep(frame->delay_micros());
        }
    }
}

void DisplayScrolling(const Magick::Image &img, int scroll_delay_ms,
                      int display_width, int display_height,
                      int offset_x, int offset_y, int offset_z,
                      int fd) {
    // Create a copy from which it is faster to extract relevant content.
    UDPFlaschenTaschen copy(-1, img.columns(), img.rows());
    CopyImage(img, &copy);
    
    UDPFlaschenTaschen display(fd, display_width, img.rows());
    display.SetOffset(offset_x, offset_y, offset_z);
    while (!interrupt_received) {
        for (int start = 0; start < copy.width(); ++start) {
            if (interrupt_received) break;
            for (int y = 0; y < display.height(); ++y) {
                for (int x = 0; x < display.width(); ++x) {
                    display.SetPixel(x, y, copy.GetPixel(x + start, y));
                }
            }
            display.Send();
            usleep(scroll_delay_ms * 1000);
        }
    }
}


static int usage(const char *progname) {
    fprintf(stderr, "usage: %s [options] <image>\n", progname);
    fprintf(stderr, "Options:\n"
            "\t-g <width>x<height>[+<off_x>+<off_y>[+<layer>]] : Output geometry. Default 20x20+0+0+0\n"
            "\t-h <host>       : host (default: flaschen-taschen.local)\n"
            "\t-s[<ms>]        : scroll horizontally (optionally: delay ms; default 60).\n");
    return 1;
}

int main(int argc, char *argv[]) {
    Magick::InitializeMagick(*argv);

    bool do_scroll = false;
    int width = 20;
    int height = 20;
    int off_x = 0;
    int off_y = 0;
    int off_z = 0;
    int scroll_delay_ms = 50;
    const char *host = "flaschen-taschen.local";

    int opt;
    while ((opt = getopt(argc, argv, "g:h:s::")) != -1) {
        switch (opt) {
        case 'g':
            if (sscanf(optarg, "%dx%d%d%d%d", &width, &height, &off_x, &off_y, &off_z)
                < 2) {
                fprintf(stderr, "Invalid size spec '%s'", optarg);
                return usage(argv[0]);
            }
            break;
        case 'h':
            host = strdup(optarg); // leaking. Ignore.
            break;
        case 's':
            do_scroll = true;
            if (optarg != NULL) {
                scroll_delay_ms = atoi(optarg);
                if (scroll_delay_ms < 5) scroll_delay_ms = 5;
            }
            break;
        default:
            return usage(argv[0]);
        }
    }
    
    if (width < 1 || height < 1) {
        fprintf(stderr, "%dx%d is a rather unusual size\n", width, height);
        return usage(argv[0]);
    }

    if (optind >= argc) {
        fprintf(stderr, "Expected image filename.\n");
        return usage(argv[0]);
    }

    int fd = OpenFlaschenTaschenSocket(host);
    if (fd < 0) {
        fprintf(stderr, "Cannot connect.");
        return 1;
    }
    const char *filename = argv[optind];

    std::vector<Magick::Image> frames;
    if (!LoadImageAndScale(filename, width, height, do_scroll, &frames)) {
        return 1;
    }

    if (do_scroll && frames.size() > 1) {
        fprintf(stderr, "This is an animated image format, "
                "scrolling does not make sense\n");
        return 1;
    }

    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    if (do_scroll) {
        DisplayScrolling(frames[0], scroll_delay_ms,
                         width, height, off_x, off_y, off_z, fd);
    } else {
        DisplayAnimation(frames, width, height, off_x, off_y, off_z, fd);
    }

    // Don't let leftovers cover up content.
    if (interrupt_received && off_z > 0) {
        UDPFlaschenTaschen clear_screen(fd, width, height);
        clear_screen.SetOffset(off_x, off_y, off_z);
        clear_screen.Send();
    }

    close(fd);

    fprintf(stderr, "Exit.\n");
    return 0;
}
