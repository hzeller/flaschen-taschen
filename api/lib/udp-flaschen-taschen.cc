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
#include <sys/uio.h>
#include <unistd.h>

#include <algorithm>

#define DEFAULT_FT_DISPLAY_HOST "ft.noise"

static const int kFlaschenTaschenHeaderReserve = 64;  // PPM header

int OpenFlaschenTaschenSocket(const char *host) {
    if (host == NULL) {
        host = getenv("FT_DISPLAY");     // Take from environment.
    }
    if (host == NULL || strlen(host) == 0) {
        host = DEFAULT_FT_DISPLAY_HOST; // Fallback.
    }
    char *host_copy = NULL;
    const char *port = "1337";
    const char *colon_pos;
    if ((colon_pos = strchr(host, ':')) != NULL) {
        port = colon_pos + 1;
        host_copy = strdup(host);
        host_copy[colon_pos - host] = '\0';
        host = host_copy;
    }
    struct addrinfo addr_hints = {};
    addr_hints.ai_family = AF_INET;
    addr_hints.ai_socktype = SOCK_DGRAM;

    struct addrinfo *addr_result = NULL;
    int rc;
    if ((rc = getaddrinfo(host, port, &addr_hints, &addr_result)) != 0) {
        fprintf(stderr, "Resolving '%s' (port %s): %s\n", host, port,
                gai_strerror(rc));
        free(host_copy);
        return -1;
    }
    free(host_copy);
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

UDPFlaschenTaschen::UDPFlaschenTaschen(int socket, int width, int height,
                                       size_t max_udp_size)
    : fd_(socket), width_(width), height_(height),
      pixel_buffer_(new Color [ width_ * height ]),
      max_udp_size_(65507) {
    SetMaxUDPPacketSize(max_udp_size);

    // Allow override with environment variable.
    if (getenv("FT_UDP_SIZE")) {
        SetMaxUDPPacketSize(atoi(getenv("FT_UDP_SIZE")));
    }

    SetOffset(0, 0, 0);
    Clear();
}

UDPFlaschenTaschen::UDPFlaschenTaschen(const UDPFlaschenTaschen& other)
    : fd_(other.fd_), width_(other.width_), height_(other.height_),
      pixel_buffer_(new Color [ width_ * height_ ]),
      max_udp_size_(other.max_udp_size_) {
    SetOffset(other.off_x_, other.off_y_, other.off_z_);
    memcpy(pixel_buffer_, other.pixel_buffer_, width_ * height_ * 3);
}

UDPFlaschenTaschen::~UDPFlaschenTaschen() { delete [] pixel_buffer_; }

bool UDPFlaschenTaschen::SetMaxUDPPacketSize(size_t packet_size) {
    if (packet_size > 65507) {
        fprintf(stderr, "Attempt to set UDP packet size beyond 65507 bytes "
                "(%d).\n", (int) packet_size);
        return false;
    }
    const size_t row_size = 3 * width_;
    if ((packet_size - kFlaschenTaschenHeaderReserve) / row_size == 0) {
        fprintf(stderr, "Attempt to set UDP packet size below minimum %d "
                "bytes needed for this canvas. Keeping %d.\n",
                (int) (row_size + kFlaschenTaschenHeaderReserve),
                (int) max_udp_size_);
        return false;
    }
    max_udp_size_ = packet_size;
    return true;
}

void UDPFlaschenTaschen::Clear() {
    bzero(pixel_buffer_, width_ * height_ * sizeof(Color));
}

void UDPFlaschenTaschen::Fill(const Color &c) {
    if (c.is_black()) {
        Clear();  // cheaper
    } else {
        std::fill(pixel_buffer_, pixel_buffer_ + width_*height_, c);
    }
}

void UDPFlaschenTaschen::SetOffset(int off_x, int off_y, int off_z){
    off_x_ = off_x;
    off_y_ = off_y;
    off_z_ = off_z;
}

void UDPFlaschenTaschen::SetPixel(int x, int y, const Color &col) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
    pixel_buffer_[x + y * width_] = col;
}

const Color &UDPFlaschenTaschen::GetPixel(int x, int y) const {
    return pixel_buffer_[(x % width_) + (y % height_) * width_];
}

void UDPFlaschenTaschen::Send(int fd) const {
    const int kMaxDataLen = max_udp_size_ - kFlaschenTaschenHeaderReserve;
    const size_t row_size = 3 * width_;
    const int max_send_height = kMaxDataLen / row_size;
    assert(max_send_height > 0);  // UDP needs to be able to fit at least 1 row

    char header_buffer[kFlaschenTaschenHeaderReserve];
    char *send_buffer = (char*)pixel_buffer_;
    int rows = height_;
    int tile_offset = 0;
    while (rows) {
        const int send_h = (rows < max_send_height) ? rows : max_send_height;
        int header_len = snprintf(header_buffer, sizeof(header_buffer),
                                  "P6\n%d %d\n#FT: %d %d %d\n255\n",
                                  width_, send_h,
                                  off_x_, off_y_ + tile_offset, off_z_);

        struct iovec iov[2];
        iov[0].iov_base = header_buffer;
        iov[0].iov_len = header_len;
        iov[1].iov_base = send_buffer;
        iov[1].iov_len = send_h * row_size;

        if (writev(fd, iov, 2) < 0) {
            perror("Error sending packet.");
        }

        rows -= send_h;
        tile_offset += send_h;
        send_buffer += send_h * row_size;
    }
}

UDPFlaschenTaschen* UDPFlaschenTaschen::Clone() const {
    return new UDPFlaschenTaschen(*this);
}
