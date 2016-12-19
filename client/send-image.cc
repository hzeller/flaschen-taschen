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
static void InterruptHandler(int) {
  interrupt_received = true;
}

namespace {
::Color ConvertColor(const Magick::Color &c, float brightness_factor) {
    return ::Color(brightness_factor * ScaleQuantumToChar(c.redQuantum()),
                   brightness_factor * ScaleQuantumToChar(c.greenQuantum()),
                   brightness_factor * ScaleQuantumToChar(c.blueQuantum()));
}

void CopyImage(const Magick::Image &img, bool do_center, float brightness_factor,
               FlaschenTaschen *result) {
    assert(result->width() >= (int) img.columns()
           && result->height() >= (int) img.rows());
    const int x_offset = do_center ? (result->width() - img.columns()) / 2 : 0;
    const int y_offset = do_center ? (result->height() - img.rows()) / 2 : 0;
    for (size_t y = 0; y < img.rows(); ++y) {
        for (size_t x = 0; x < img.columns(); ++x) {
            const Magick::Color &c = img.pixelColor(x, y);
            if (c.alphaQuantum() >= 255)
                continue;
            result->SetPixel(x + x_offset, y + y_offset,
                             ConvertColor(c, brightness_factor));
        }
    }
}

// Frame already prepared as the buffer to be sent over so that animation
// and re-sizing operations don't have to be done online. Also knows about the
// animation delay.
class PreprocessedFrame {
public:
    PreprocessedFrame(const Magick::Image &img, bool do_center,
                      float brightness_factor,
                      const UDPFlaschenTaschen &display)
        : content_(display.Clone()) {
        int delay_time = img.animationDelay();  // in 1/100s of a second.
        if (delay_time < 1) delay_time = 10;
        delay_micros_ = delay_time * 10000;
        CopyImage(img, do_center, brightness_factor, content_);
    }
    ~PreprocessedFrame() { delete content_; }

    void Send() { content_->Send(); }
    int delay_micros() const { return delay_micros_; }

private:
    UDPFlaschenTaschen *content_;
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
                      float bright, bool do_center, time_t end_time,
                      const UDPFlaschenTaschen &display) {
    std::vector<PreprocessedFrame*> frames;
    // Convert to preprocessed frames.
    for (size_t i = 0; i < image_sequence.size(); ++i) {
        frames.push_back(new PreprocessedFrame(image_sequence[i], do_center,
                                               bright, display));
    }

    for (unsigned int i = 0; !interrupt_received; ++i) {
        if (time(NULL) > end_time)
            break;
        if (i == frames.size()) i = 0;
        PreprocessedFrame *frame = frames[i];
        frame->Send();  // Simple. just send it.
        if (frames.size() == 1) {
            return;  // We are done.
        } else {
            usleep(frame->delay_micros());
        }
    }
    // Leaking PreprocessedFrames. Don't care.
}

void DisplayScrolling(const Magick::Image &img, int scroll_delay_ms,
                      float brightness_factor, time_t end_time,
                      UDPFlaschenTaschen *display) {
    // Create a copy from which it is faster to extract relevant content.
    UDPFlaschenTaschen copy(-1, img.columns(), img.rows());
    CopyImage(img, false, brightness_factor, &copy);

    while (!interrupt_received && time(NULL) <= end_time) {
        for (int start = 0; start < copy.width(); ++start) {
            if (interrupt_received) break;
            for (int y = 0; y < display->height(); ++y) {
                for (int x = 0; x < display->width(); ++x) {
                    display->SetPixel(x, y, copy.GetPixel(x + start, y));
                }
            }
            display->Send();
            usleep(scroll_delay_ms * 1000);
        }
    }
}


static int usage(const char *progname) {
    fprintf(stderr, "usage: %s [options] <image>\n", progname);
    fprintf(stderr, "Options:\n"
            "\t-g <width>x<height>[+<off_x>+<off_y>[+<layer>]] : Output geometry. Default 45x35+0+0+0\n"
            "\t-l <layer>      : Layer 0..15. Default 0 (note if also given in -g, then last counts)\n"
            "\t-h <host>       : Flaschen-Taschen display hostname.\n"
            "\t-c              : Center image in available space.\n"
            "\t-s[<ms>]        : Scroll horizontally (optionally: delay ms; default 60).\n"
            "\t-b<brighness%%>  : Brightness percent (default 100)\n"
            "\t-t<timeout>     : Only display for this long\n\n"
            "\t-C              : Just clear given area and exit.\n");
    return 1;
}

int main(int argc, char *argv[]) {
    Magick::InitializeMagick(*argv);

    bool do_scroll = false;
    bool do_clear_screen = false;
    bool do_center = false;
    int width = 45;
    int height = 35;
    int off_x = 0;
    int off_y = 0;
    int off_z = 0;
    int scroll_delay_ms = 50;
    int brighness_percent = 100;
    const char *host = NULL;
    int timeout = 1000000;

    int opt;
    while ((opt = getopt(argc, argv, "g:h:s::Cl:b:t:c")) != -1) {
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
        case 't':
            timeout = atoi(optarg);
            break;
        case 'l':
            if (sscanf(optarg, "%d", &off_z) != 1 || off_z < 0 || off_z >= 16) {
                fprintf(stderr, "Invalid layer '%s'\n", optarg);
                return usage(argv[0]);
            }
            break;
        case 's':
            do_scroll = true;
            if (optarg != NULL) {
                scroll_delay_ms = atoi(optarg);
                if (scroll_delay_ms < 5) scroll_delay_ms = 5;
                if (scroll_delay_ms > 999) scroll_delay_ms = 999;
            }
            break;
        case 'b':
            brighness_percent = atoi(optarg);
            break;
        case 'c':
            do_center = true;
            break;
        case 'C':
            do_clear_screen = true;
            break;
        default:
            return usage(argv[0]);
        }
    }

    if (width < 1 || height < 1) {
        fprintf(stderr, "%dx%d is a rather unusual size\n", width, height);
        return usage(argv[0]);
    }

    if (brighness_percent < 0) brighness_percent = 0;
    if (brighness_percent > 100) brighness_percent = 100;

    if (!do_clear_screen && optind >= argc) {
        fprintf(stderr, "Expected image filename.\n");
        return usage(argv[0]);
    }

    int fd = OpenFlaschenTaschenSocket(host);
    if (fd < 0) {
        fprintf(stderr, "Cannot connect.\n");
        return 1;
    }

    UDPFlaschenTaschen display(fd, width, height);
    display.SetOffset(off_x, off_y, off_z);

    if (do_clear_screen) {
        display.Send();
        fprintf(stderr, "Cleared screen area %dx%d%+d%+d%+d.\n",
                width, height, off_x, off_y, off_z);
        if (optind < argc) {
            fprintf(stderr, "FYI: You also supplied filename %s. "
                    "Ignoring that for clear-screen operation.\n",
                    argv[optind]);
        }
        return 0;
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

    const float bright = brighness_percent / 100.0f;
    const time_t end_time = time(NULL) + timeout;
    if (do_scroll) {
        DisplayScrolling(frames[0], scroll_delay_ms, bright, end_time, &display);
    } else {
        DisplayAnimation(frames, bright, do_center, end_time, display);
    }

    // Don't let leftovers cover up content.
    if ((interrupt_received && off_z > 0) || time(NULL) >= end_time) {
        display.Clear();
        display.Send();
    }

    close(fd);

    fprintf(stderr, "Exit.\n");
    return 0;
}
