// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
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

#include <arpa/inet.h>
#include <ctype.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "composite-flaschen-taschen.h"
#include "ft-thread.h"
#include "servers.h"

struct ImageInfo {
    int width;
    int height;
    int range;
    int offset_x;
    int offset_y;
    int layer;
};

static const char *skipWhitespace(const char *buffer, const char *end) {
    for (;;) {
        while (buffer < end && isspace(*buffer))
            ++buffer;
        if (buffer >= end)
            return NULL;
        if (*buffer == '#') {
            while (buffer < end && *buffer != '\n') // read to end of line.
                ++buffer;
            continue;  // Back to whitespace eating.
        }
        return buffer;
    }
}

// Read next number. Start reading at *start; modifies the *start pointer
// to point to the character just after the decimal number or NULL if reading
// was not successful.
static int readNextNumber(const char **start, const char *end) {
    const char *start_number = skipWhitespace(*start, end);
    if (start_number == NULL) { *start = NULL; return 0; }
    char *end_number = NULL;
    int result = strtol(start_number, &end_number, 10);
    if (end_number == start_number) { *start = NULL; return 0; }
    *start = end_number;
    return result;
}

// Given an image buffer containing either a PPM or raw framebuffer, extract the
// width and height of the image (if available) and return pointer to start
// of image data.
// Image info is left untouched when image does not contain any
// information.
static const char *GetImageData(const char *in_buffer, size_t buf_len,
                                   struct ImageInfo *info) {
    if (in_buffer[0] != 'P' || in_buffer[1] != '6' ||
        (!isspace(in_buffer[2]) && in_buffer[2] != '#')) {
        return in_buffer;  // raw image. No P6 magic header.
    }
    const char *const end = in_buffer + buf_len;
    const char *parse_buffer = in_buffer + 2;
    const int width = readNextNumber(&parse_buffer, end);
    if (parse_buffer == NULL) return in_buffer;
    const int height = readNextNumber(&parse_buffer, end);
    if (parse_buffer == NULL) return in_buffer;
    const int range = readNextNumber(&parse_buffer, end);
    if (parse_buffer == NULL) return in_buffer;
    if (!isspace(*parse_buffer++)) return in_buffer;   // last char before data
    // Now make sure that the rest of the buffer still makes sense
    const size_t expected_image_data = width * height * 3;
    const size_t actual_data = end - parse_buffer;
    if (actual_data < expected_image_data)
        return in_buffer;   // Uh, not enough data.
    if (actual_data > expected_image_data) {
        // Our extension: at the end of the binary data, we provide an optional
        // offset. We can't store it in the header, as it is fixed in number
        // of fields. But nobody cares what is at the end of the buffer.
        const char *offset_data = parse_buffer + expected_image_data;
        info->offset_x = readNextNumber(&offset_data, end);
        if (offset_data != NULL) {
            info->offset_y = readNextNumber(&offset_data, end);
        }
        if (offset_data != NULL) {
            info->layer = readNextNumber(&offset_data, end);
        }
    }
    info->width = width;
    info->height = height;
    info->range = range;
    return parse_buffer;
}

// public interface
static int server_socket = -1;
bool udp_server_init(int port) {
    if ((server_socket = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("IPv6 enabled ? While reating listen socket");
        return false;
    }
    int opt = 0;   // Unset IPv6-only, in case it is set. Best effort.
    setsockopt(server_socket, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));

    struct sockaddr_in6 addr = {0};
    addr.sin6_family = AF_INET6;
    addr.sin6_addr = in6addr_any;
    addr.sin6_port = htons(port);
    if (bind(server_socket, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("bind");
        return false;
    }

    fprintf(stderr, "UDP-server: ready to listen on %d\n", port);
    return true;
}

void udp_server_run_blocking(CompositeFlaschenTaschen *display,
                             ft::Mutex *mutex) {
    static const int kBufferSize = 65535;  // maximum UDP has to offer.
    char *packet_buffer = new char[kBufferSize];
    bzero(packet_buffer, kBufferSize);

    for (;;) {
        ssize_t received_bytes = recvfrom(server_socket,
                                          packet_buffer, kBufferSize,
                                          0, NULL, 0);
        if (received_bytes < 0) {
            perror("Trouble receiving.");
            break;
        }

        ImageInfo img_info = {0};
        img_info.width = display->width();  // defaults.
        img_info.height = display->height();

        const char *pixel_pos = GetImageData(packet_buffer, received_bytes,
                                             &img_info);
        mutex->Lock();
        display->SetLayer(img_info.layer);
        for (int y = 0; y < img_info.height; ++y) {
            for (int x = 0; x < img_info.width; ++x) {
                Color c;
                c.r = *pixel_pos++;
                c.g = *pixel_pos++;
                c.b = *pixel_pos++;
                display->SetPixel(x + img_info.offset_x,
                                  y + img_info.offset_y,
                                  c);
            }
        }
        display->Send();
        display->SetLayer(0);  // Back to sane default.
        mutex->Unlock();
    }
    delete [] packet_buffer;
}
