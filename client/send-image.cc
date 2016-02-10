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
// $ sudo aptitude install libmagick++-dev

#include <math.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>

#include <vector>
#include <Magick++.h>
#include <magick/image.h>
#include "udp-flaschen-taschen.h"

namespace {
// Frame already prepared as the buffer to be sent over so that animation
// and re-sizing operations don't have to be done online. Also knows about the
// animation delay.
class PreprocessedFrame {
public:
    PreprocessedFrame(const Magick::Image &img, int width, int height)
        : width_(width), content_(-1, width, height) {
        assert(width >= (int) img.columns() && height >= (int) img.rows());
        int delay_time = img.animationDelay();  // in 1/100s of a second.
        if (delay_time < 1) delay_time = 1;
        delay_micros_ = delay_time * 10000;

        for (size_t y = 0; y < img.rows(); ++y) {
            for (size_t x = 0; x < img.columns(); ++x) {
                const Magick::Color &c = img.pixelColor(x, y);
                if (c.alphaQuantum() >= 255)
                    continue;
                const ::Color ft_col(ScaleQuantumToChar(c.redQuantum()),
                                     ScaleQuantumToChar(c.greenQuantum()),
                                     ScaleQuantumToChar(c.blueQuantum()));
                content_.SetPixel(x, y, ft_col);
            }
        }
    }
 
    int width() const { return width_; }

    void Send(int fd) { content_.Send(fd); }
    void ExtractCrop(int offset_x, int offset_y, FlaschenTaschen *target) {
        for (int x = 0; x < target->width(); ++x) {
            for (int y = 0; y < target->height(); ++y) {
                target->SetPixel(x, y,
                                 content_.GetPixel(x + offset_x, y + offset_y));
            }
        }
    }

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
// Scale, so that it fits in "width" and "height" and store in "image_sequence".
// If this is a still image, "image_sequence" will contain one image, otherwise
// all animation frames.
static bool LoadAnimationAndScale(const char *filename,
                                  int target_width, int target_height,
                                  bool scroll,
                                  std::vector<PreprocessedFrame*> *result) {
    std::vector<Magick::Image> frames;
    fprintf(stderr, "Read image...\n");
    readImages(&frames, filename);
    if (frames.size() == 0) {
        fprintf(stderr, "No image found.");
        return false;
    }

    std::vector<Magick::Image> image_sequence;
    // Put together the animation from single frames. GIFs can have nasty
    // disposal modes, but they are handled nicely by coalesceImages()
    if (frames.size() > 1) {
        if (scroll) {
            fprintf(stderr, "This is an animated image format, "
                    "scrolling does not make sense\n");
            return false;
        }
        fprintf(stderr, "Assembling animation with %d frames.\n",
                (int)frames.size());
        Magick::coalesceImages(&image_sequence, frames.begin(), frames.end());
    } else {
        image_sequence.push_back(frames[0]);   // just a single still image.
    }

    const int img_width = (int)image_sequence[0].columns();
    const int img_height = (int)image_sequence[0].rows();
    if (scroll) {
        target_width = (int) (1.0*img_width * target_height / img_height);
    }
    fprintf(stderr, "Scale ... %dx%d -> %dx%d\n",
            img_width, img_height, target_width, target_height);
    for (size_t i = 0; i < image_sequence.size(); ++i) {
        image_sequence[i].scale(Magick::Geometry(target_width, target_height));
    }

    // Now, convert to preprocessed frames.
    for (size_t i = 0; i < image_sequence.size(); ++i) {
        result->push_back(new PreprocessedFrame(image_sequence[i],
                                                target_width, target_height));
    }

    return true;
}

void DisplayAnimation(const std::vector<PreprocessedFrame*> &frames,
                      int display_width, int display_height, int fd) {
    fprintf(stderr, "Display.\n");
    for (unsigned int i = 0; /*forever*/; ++i) {
        PreprocessedFrame *frame = frames[i % frames.size()];
        const bool requires_scroll = (frame->width() > display_width);
        if (!requires_scroll) {
            frame->Send(fd);  // Simple. just send it.
        } else {
            // Width is larger than what we have. Scroll.
            UDPFlaschenTaschen scroll_buffer(fd, display_width, display_height);
            for (int x_off = 0; x_off < frame->width(); ++x_off) {
                frame->ExtractCrop(x_off, 0, &scroll_buffer);
                scroll_buffer.Send();
                usleep(60 * 1000);
            }
        }
        if (frames.size() == 1 && !requires_scroll) {
            return;  // We are done.
        } else {
            usleep(frame->delay_micros());
        }
    }
}


static int usage(const char *progname) {
    fprintf(stderr, "usage: %s [options] <image>\n", progname);
    fprintf(stderr, "Options:\n"
            "\t-D <width>x<height> : Output dimension. Default 10x10\n"
            "\t-h <host>           : host (default: flaschen-taschen.local\n"
            "\t-s                  : scroll horizontally.\n");
    return 1;
}

int main(int argc, char *argv[]) {
    Magick::InitializeMagick(*argv);

    bool scroll = false;
    int width = 10;
    int height = 10;
    const char *host = "flaschen-taschen.local";

    int opt;
    while ((opt = getopt(argc, argv, "D:h:s")) != -1) {
        switch (opt) {
        case 'D':
            if (sscanf(optarg, "%dx%d", &width, &height) != 2) {
                fprintf(stderr, "Invalid size spec '%s'", optarg);
                return usage(argv[0]);
            }
            break;
        case 'h':
            host = strdup(optarg); // leaking. Ignore.
            break;
        case 's':
            scroll = true;
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

    std::vector<PreprocessedFrame*> frames;
    if (!LoadAnimationAndScale(filename, width, height, scroll, &frames)) {
        return 1;
    }

    DisplayAnimation(frames, width, height, fd);

    close(fd);
    return 0;
}
