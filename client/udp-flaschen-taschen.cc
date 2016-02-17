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

UDPFlaschenTaschen::UDPFlaschenTaschen(int socket, int width, int height)
    : fd_(socket), width_(width), height_(height),
      buf_size_(width * height * sizeof(Color)),
      buffer_(new Color [ buf_size_ ]) {
    bzero(buffer_, buf_size_);
}
UDPFlaschenTaschen::~UDPFlaschenTaschen() { delete [] buffer_; }

void UDPFlaschenTaschen::Clear() {
    bzero(buffer_, buf_size_);
}

void UDPFlaschenTaschen::SetPixel(int x, int y, const Color &col) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
    memcpy(buffer_ + x + y * width_, &col, sizeof(Color));
}

const Color &UDPFlaschenTaschen::GetPixel(int x, int y) {
    return buffer_[(x % width_) + (y % height_) * width_];
}

void UDPFlaschenTaschen::UDPFlaschenTaschen::Send(int fd) {
    // Some fudging to make the compiler shut up about non-used return value
    if (write(fd, buffer_, buf_size_) < 0) return;
}
