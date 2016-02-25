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
//
//
// Our format has the same header and data as a P6 PPM format.
// However, we add an optional footer with offset_x and offset_y where
// to display the PPM image.
// This is to
//   * be compatible with regular PPM: it can be read, but footer is ignored.
//   * it couldn't have been put in the header, as that is already strictly
//     defined to contain exactly three decimal numbers.
//
#include "udp-flaschen-taschen.h"

#include <assert.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int OpenFlaschenTaschenSocket(const char *host) {
    struct addrinfo addr_hints = {0};
    addr_hints.ai_family = AF_INET;
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

// Let's have a fixed-size footer for fixed buffer calculation.
static const int kFooterLen = strlen("\n0001 0001 0001\n") + 1; // offsets.

UDPFlaschenTaschen::UDPFlaschenTaschen(int socket, int width, int height)
    : fd_(socket), width_(width), height_(height) {
    char header[64];
    int header_len = snprintf(header, sizeof(header),
                              "P6\n%d %d\n255\n", width, height);
    buf_size_ = header_len + width_ * height_ * sizeof(Color) + kFooterLen;
    buffer_ = new char[buf_size_];
    bzero(buffer_, buf_size_);
    strcpy(buffer_, header);
    pixel_buffer_start_ = reinterpret_cast<Color*>(buffer_ + header_len);
    footer_start_ = buffer_ + buf_size_ - kFooterLen;
    SetOffset(0, 0);
}
UDPFlaschenTaschen::~UDPFlaschenTaschen() { delete [] buffer_; }

void UDPFlaschenTaschen::Clear() {
    bzero(pixel_buffer_start_, width_ * height_ * sizeof(Color));
}

void UDPFlaschenTaschen::SetOffset(int offset_x, int offset_y, int offset_z){
    // Our extension to the PPM format adds additional information after the
    // image data.
    snprintf(footer_start_, kFooterLen, "\n%4d %4d %4d\n",
             offset_x, offset_y, offset_z);
}

void UDPFlaschenTaschen::SetPixel(int x, int y, const Color &col) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
    pixel_buffer_start_[x + y * width_] = col;
}

const Color &UDPFlaschenTaschen::GetPixel(int x, int y) {
    return pixel_buffer_start_[(x % width_) + (y % height_) * width_];
}

void UDPFlaschenTaschen::Send(int fd) {
    // Some fudging to make the compiler shut up about non-used return value
    if (write(fd, buffer_, buf_size_) < 0) return;
}

UDPFlaschenTaschen* UDPFlaschenTaschen::Clone() const {
    UDPFlaschenTaschen *result = new UDPFlaschenTaschen(fd_, width_, height_);
    memcpy(result->buffer_, buffer_, buf_size_);
    return result;
}
