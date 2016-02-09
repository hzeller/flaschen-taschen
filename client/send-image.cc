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

namespace {
// Frame already prepared as the buffer to be sent over so that animation
// and re-sizing operations don't have to be done online. Also knows about the
// animation delay.
class PreprocessedFrame {
public:
    PreprocessedFrame(const Magick::Image &img, int width, int height) {
        buffer_.resize(3 * height * width);
        assert(width >= (int) img.columns() && height >= (int) img.rows());
        int delay_time = img.animationDelay();  // in 1/100s of a second.
        if (delay_time < 1) delay_time = 1;
        delay_micros_ = delay_time * 10000;

        std::string::iterator write_pos = buffer_.begin();
        for (size_t y = 0; y < img.rows(); ++y) {
            for (size_t x = 0; x < img.columns(); ++x) {
                const Magick::Color &c = img.pixelColor(x, y);
                if (c.alphaQuantum() < 256) {
                    *(write_pos + 0) = ScaleQuantumToChar(c.redQuantum());
                    *(write_pos + 1) = ScaleQuantumToChar(c.greenQuantum());
                    *(write_pos + 2) = ScaleQuantumToChar(c.blueQuantum());
                }
                write_pos += 3;
            }
        }
    }

    const std::string &data() const { return buffer_; }

    int delay_micros() const {
        return delay_micros_;
    }

private:
    std::string buffer_;
    int delay_micros_;
};
}  // end anonymous namespace

// Load still image or animation.
// Scale, so that it fits in "width" and "height" and store in "image_sequence".
// If this is a still image, "image_sequence" will contain one image, otherwise
// all animation frames.
static bool LoadAnimationAndScale(const char *filename, int width, int height,
                                  std::vector<Magick::Image> *image_sequence) {
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
        Magick::coalesceImages(image_sequence, frames.begin(), frames.end());
    } else {
        image_sequence->push_back(frames[0]);   // just a single still image.
    }

    fprintf(stderr, "Scale ... %dx%d -> %dx%d\n",
            (int)(*image_sequence)[0].columns(),
            (int)(*image_sequence)[0].rows(),
            width, height);
    for (size_t i = 0; i < image_sequence->size(); ++i) {
        (*image_sequence)[i].scale(Magick::Geometry(width, height));
    }
    return true;
}

static void DisplayAnimation(const std::vector<PreprocessedFrame*> &frames,
                             int fd) {
    fprintf(stderr, "Display.\n");
    for (unsigned int i = 0; /*forever*/; ++i) {
        const PreprocessedFrame *frame = frames[i % frames.size()];
        if (write(fd, frame->data().data(), frame->data().size())
            != (ssize_t)frame->data().size()) {
            fprintf(stderr, "Couldn't send full image.");
        }
        if (frames.size() == 1) {
            return;  // We are done.
        } else {
            usleep(frame->delay_micros());
        }
    }
}

static int OpenUDPSocket(const char *host) {
    struct addrinfo addr_hints = {0};
    addr_hints.ai_family = AF_UNSPEC;
    addr_hints.ai_socktype = SOCK_DGRAM;

    struct addrinfo *addr_result = NULL;
    int rc;
    if ((rc = getaddrinfo(host, "1337", &addr_hints, &addr_result)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
        return -1;
    }
    if (addr_result == NULL)
        return -1;
    int fd = socket(addr_result->ai_family,
                    addr_result->ai_socktype,
                    addr_result->ai_protocol);
    if (fd >= 0 &&
        connect(fd, addr_result->ai_addr, addr_result->ai_addrlen) < 0) {
        perror("connect()");
        close(fd);
        fd = -1;
    }

    freeaddrinfo(addr_result);
    return fd;
}

static int usage(const char *progname) {
    fprintf(stderr, "usage: %s [options] <image>\n", progname);
    fprintf(stderr, "Options:\n"
            "\t-D <width>x<height> : Output dimension. Default 10x10\n"
            "\t-h <host>           : host (default: flaschen-taschen.local\n");
    return 1;
}

int main(int argc, char *argv[]) {
    Magick::InitializeMagick(*argv);

    int width = 10;
    int height = 10;
    const char *host = "flaschen-taschen.local";

    int opt;
    while ((opt = getopt(argc, argv, "D:h:")) != -1) {
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

    int fd = OpenUDPSocket(host);
    if (fd < 0) {
        fprintf(stderr, "Cannot connect.");
        return 1;
    }
    const char *filename = argv[optind];

    std::vector<Magick::Image> sequence_pics;
    if (!LoadAnimationAndScale(filename, width, height, &sequence_pics)) {
        return 1;
    }

    std::vector<PreprocessedFrame*> frames;
    for (size_t i = 0; i < sequence_pics.size(); ++i) {
        frames.push_back(new PreprocessedFrame(sequence_pics[i],
                                               width, height));
    }

    DisplayAnimation(frames, fd);

    close(fd);
    return 0;
}
