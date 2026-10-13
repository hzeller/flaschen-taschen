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

#include "led-flaschen-taschen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>

// Max pixels per Colorlight packet
#define CL_MAX_PIXELS_PER_PACKET 497
#define CL_BYTES_PER_PIXEL 3

// Colorlight protocol constants
static const uint8_t CL_DST_MAC[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
static const uint8_t CL_SRC_MAC[6] = {0x22, 0x22, 0x33, 0x44, 0x55, 0x66};

// Colorlight frame layout (NOT standard Ethernet!):
//   Bytes 0-5:   Destination MAC
//   Bytes 6-11:  Source MAC
//   Byte 12:     Packet Type (0x55=pixel, 0x01=sync, 0x0A=brightness)
//   Byte 13+:    Payload data (NO standard 2-byte EtherType!)
#define CL_PACKET_TYPE_OFFSET   12
#define CL_DATA_OFFSET          13

#define CL_PIXEL_HEADER_SIZE    8

#define CL_SYNC_PACKET_SIZE     112 // 12 MAC + 1 type + 99 data
#define CL_BRIG_PACKET_SIZE     77  // 12 MAC + 1 type + 64 data

// Max packet: 13 header + 8 pixel header + 497*3 BGR = 1512 bytes
#define CL_MAX_PACKET_SIZE (CL_DATA_OFFSET + CL_PIXEL_HEADER_SIZE + CL_MAX_PIXELS_PER_PACKET * CL_BYTES_PER_PIXEL)
static uint8_t row_packet_buf[CL_MAX_PACKET_SIZE];

static struct sockaddr_ll sock_addr;

ColorlightFlaschenTaschen::ColorlightFlaschenTaschen(const char* interface_name, int width, int height, uint8_t brightness)
    : width_(width), height_(height), brightness_(brightness),
      sock_fd_(-1), frame_count_(0), frame_data_(NULL) {

    fprintf(stderr, "Running with %dx%d resolution and %d brightness\n", width_, height_, brightness_);

    int needed = width_ * height_ * CL_BYTES_PER_PIXEL;
    frame_data_ = (uint8_t*)malloc(needed);
    if (!frame_data_) {
        perror("Unable to allocate frame buffer");
        return;
    }

    // Create raw socket
    sock_fd_ = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock_fd_ < 0) {
        perror("Failed to create raw socket. Run with sudo or set CAP_NET_RAW");
        return;
    }

    // Get interface index
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ - 1);
    if (ioctl(sock_fd_, SIOCGIFINDEX, &ifr) < 0) {
        perror("Failed to get interface index");
        close(sock_fd_);
        sock_fd_ = -1;
        return;
    }

    int ifindex = ifr.ifr_ifindex;

    // Bind to interface
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sll_family = AF_PACKET;
    sock_addr.sll_protocol = htons(ETH_P_ALL);
    sock_addr.sll_ifindex = ifindex;
    sock_addr.sll_halen = 6;
    memcpy(sock_addr.sll_addr, CL_DST_MAC, 6);

    if (bind(sock_fd_, (struct sockaddr*)&sock_addr, sizeof(sock_addr)) < 0) {
        perror("Failed to bind to interface");
        close(sock_fd_);
        sock_fd_ = -1;
        return;
    }

    fprintf(stderr, "Colorlight output opened on %s (ifindex=%d, fd=%d)\n",
            interface_name, ifindex, sock_fd_);
}

ColorlightFlaschenTaschen::~ColorlightFlaschenTaschen() {
    close(sock_fd_);
    sock_fd_ = -1;
    free(frame_data_);
    fprintf(stderr, "Colorlight output closed (%d frames sent)\n", frame_count_);
}

void ColorlightFlaschenTaschen::SetPixel(int x, int y, const Color &col) {
    if (!frame_data_) return;
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return;

    int idx = (y * width_ + x) * CL_BYTES_PER_PIXEL;
    frame_data_[idx + 0] = col.b;
    frame_data_[idx + 1] = col.g;
    frame_data_[idx + 2] = col.r;
}

void ColorlightFlaschenTaschen::Send() {
    if (sock_fd_ < 0) return;
    if (!frame_data_) return;

    for (int y = 0; y < height_; y++) {
        const uint8_t* row_data = frame_data_ + y * width_ * CL_BYTES_PER_PIXEL;
        send_row_data(y, row_data);
    }

    // 5ms delay before sync
    struct timespec sync_delay = {0, 5000000};
    nanosleep(&sync_delay, NULL);

    send_sync_packet();

    frame_count_++;
}

void ColorlightFlaschenTaschen::SetBrightness(uint8_t brightness) {
    brightness_ = brightness;
    if (sock_fd_ >= 0) {
        send_brightness_packet();
    }
}

// Build frame header: DST MAC + SRC MAC + packet type byte
void ColorlightFlaschenTaschen::build_frame_header(uint8_t* buf, uint8_t packet_type) {
    memcpy(buf, CL_DST_MAC, 6);
    memcpy(buf + 6, CL_SRC_MAC, 6);
    buf[CL_PACKET_TYPE_OFFSET] = packet_type;
}

// Send sync/display update packet (112 bytes)
void ColorlightFlaschenTaschen::send_sync_packet(void) {
    uint8_t packet[CL_SYNC_PACKET_SIZE];
    memset(packet, 0, sizeof(packet));

    build_frame_header(packet, 0x01);

    uint8_t* data = packet + CL_DATA_OFFSET;
    data[0] = 0x07;
    data[22] = brightness_;
    data[23] = 0x05;
    data[25] = brightness_;
    data[26] = brightness_;
    data[27] = brightness_;

    raw_send(packet, CL_SYNC_PACKET_SIZE);
}

// Send pixel data for one row
void ColorlightFlaschenTaschen::send_row_data(int row, const uint8_t* frame_data) {
    int pixels_sent = 0;

    while (pixels_sent < width_) {
        int pixels_in_packet = width_ - pixels_sent;
        if (pixels_in_packet > CL_MAX_PIXELS_PER_PACKET)
            pixels_in_packet = CL_MAX_PIXELS_PER_PACKET;

        int pixel_data_size = pixels_in_packet * CL_BYTES_PER_PIXEL;
        int packet_size = CL_DATA_OFFSET + CL_PIXEL_HEADER_SIZE + pixel_data_size;

        memset(row_packet_buf, 0, CL_DATA_OFFSET + CL_PIXEL_HEADER_SIZE);
        build_frame_header(row_packet_buf, 0x55);

        uint8_t* hdr = row_packet_buf + CL_DATA_OFFSET;
        hdr[0] = (row >> 8) & 0xFF;
        hdr[1] = row & 0xFF;
        hdr[2] = (pixels_sent >> 8) & 0xFF;
        hdr[3] = pixels_sent & 0xFF;
        hdr[4] = (pixels_in_packet >> 8) & 0xFF;
        hdr[5] = pixels_in_packet & 0xFF;
        hdr[6] = 0x08;
        hdr[7] = 0x88;

        memcpy(hdr + CL_PIXEL_HEADER_SIZE,
               frame_data + pixels_sent * CL_BYTES_PER_PIXEL,
               pixel_data_size);

        raw_send(row_packet_buf, packet_size);
        pixels_sent += pixels_in_packet;
    }
}

void ColorlightFlaschenTaschen::send_brightness_packet() {
    uint8_t packet[CL_BRIG_PACKET_SIZE];
    memset(packet, 0, sizeof(packet));

    build_frame_header(packet, 0x0A);

    uint8_t* data = packet + CL_DATA_OFFSET;
    data[0] = brightness_;
    data[1] = brightness_;
    data[2] = brightness_;
    data[3] = 0xFF;

    raw_send(packet, CL_BRIG_PACKET_SIZE);
}

// Send raw ethernet frame via AF_PACKET
void ColorlightFlaschenTaschen::raw_send(const uint8_t* data, size_t len) {
    if (sock_fd_ < 0) return;

    ssize_t written = sendto(sock_fd_, data, len, 0,
                             (struct sockaddr*)&sock_addr, sizeof(sock_addr));
    if (written < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("sendto failed");
        }
    }
}
