// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//  PixelPusher protocol implementation for LED matrix
//
//  Copyright (C) 2013 Henner Zeller <h.zeller@acm.org>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <arpa/inet.h>
#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <linux/netdevice.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stropts.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>

#include "flaschen-taschen.h"
#include "ft-thread.h"
#include "pixel-push-discovery.h"

using namespace ft;

// PixelPushPrefix :) for our logs.
#define PXP "PixelPush: "

static const char kNetworkInterface[] = "eth0";
static const uint16_t kPixelPusherDiscoveryPort = 7331;
static const uint16_t kPixelPusherListenPort = 5078;
static const uint8_t kSoftwareRevision = 122;

// The maximum packet size we accept.
// Typicall, the PixelPusher network will attempt to send smaller,
// non-fragmenting packets of size 1460; however, we would accept up to
// the UDP packet size.
static const int kMaxUDPPacketSize = 65507;

// Say we want 60Hz update and 9 packets per frame (7 strips / packet), we
// don't really need more update rate than this.
static const uint32_t kMinUpdatePeriodUSec = 16666 / 9;

int64_t CurrentTimeMicros() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t result = tv.tv_sec;
    return result * 1000000 + tv.tv_usec;
}

// Given the name of the interface, such as "eth0", fill the IP address and
// broadcast address into "header"
// Some socket and ioctl nastiness.
bool DetermineNetwork(const char *interface, DiscoveryPacketHeader *header) {
    int s;
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return false;
    }

    bool success = true;

    {
        // Get mac address for given interface name.
        struct ifreq mac_addr_query;
        strcpy(mac_addr_query.ifr_name, interface);
        if (ioctl(s, SIOCGIFHWADDR, &mac_addr_query) == 0) {
            memcpy(header->mac_address,  mac_addr_query.ifr_hwaddr.sa_data,
                   sizeof(header->mac_address));
        } else {
            perror("Getting hardware address");
            success = false;
        }
    }

    {
        struct ifreq ip_addr_query;
        strcpy(ip_addr_query.ifr_name, interface);
        if (ioctl(s, SIOCGIFADDR, &ip_addr_query) == 0) {
            struct sockaddr_in *s_in = (struct sockaddr_in *) &ip_addr_query.ifr_addr;
            memcpy(header->ip_address, &s_in->sin_addr, sizeof(header->ip_address));
        } else {
            perror("Getting IP address");
            success = false;
        }
    }

    close(s);

    // Let's print what we're sending.
    char buf[256];
    inet_ntop(AF_INET, header->ip_address, buf, sizeof(buf));
    fprintf(stderr, PXP "%s: IP: %s; MAC: ", interface, buf);
    for (int i = 0; i < 6; ++i) {
        fprintf(stderr, "%s%02x", (i == 0) ? "" : ":", header->mac_address[i]);
    }
    fprintf(stderr, "\n");

    return success;
}

// Threads deriving from this should exit Run() as soon as they see !running_
class StoppableThread : public Thread {
public:
    StoppableThread() : running_(true) {}
    virtual ~StoppableThread() { Stop(); }

    // Cause stopping wait until we're done.
    void Stop() {
        MutexLock l(&run_mutex_);
        running_ = false;
    }

protected:
    bool running() { MutexLock l(&run_mutex_); return running_; }

private:
    Mutex run_mutex_;
    bool running_;
};

// Broadcast every second the discovery protocol.
class Beacon : public StoppableThread {
public:
    Beacon(const DiscoveryPacketHeader &header,
           const PixelPusherContainer &pixel_pusher)
        : header_(header), pixel_pusher_(pixel_pusher),
          pixel_pusher_base_size_(CalcPixelPusherBaseSize(pixel_pusher_.base
                                                          ->strips_attached)),
          discovery_packet_size_(sizeof(header_)
                                 + pixel_pusher_base_size_
                                 + sizeof(pixel_pusher_.ext)),
          discovery_packet_buffer_(new uint8_t[discovery_packet_size_]),
        previous_sequence_(-1) {
        fprintf(stderr, PXP "discovery packet size: %zd\n",
                discovery_packet_size_);
    }

    virtual ~Beacon() {
        Stop();
        delete [] discovery_packet_buffer_;
    }

    void UpdatePacketStats(uint32_t seen_sequence, uint32_t update_micros) {
        MutexLock l(&mutex_);
        pixel_pusher_.base->update_period = (update_micros < kMinUpdatePeriodUSec
                                             ? kMinUpdatePeriodUSec
                                             : update_micros);
        const int32_t sequence_diff = seen_sequence - previous_sequence_ - 1;
        if (sequence_diff > 0)
            pixel_pusher_.base->delta_sequence += sequence_diff;
        previous_sequence_ = seen_sequence;
    }

    virtual void Run() {
        int s;
        if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            perror("socket");
            exit(1);    // don't worry about graceful exit.
        }

        int enable = 1;
        if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(enable)) < 0) {
            perror("enable broadcast");
            exit(1);
        }

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
        addr.sin_port = htons(kPixelPusherDiscoveryPort);

        fprintf(stderr, PXP "Starting PixelPusher discovery beacon "
                "broadcasting to port %d\n", kPixelPusherDiscoveryPort);
        struct timespec sleep_time = { 1, 0 };  // todo: tweak.
        while (running()) {
            // The header is of type 'DiscoveryPacket'.
            {
                MutexLock l(&mutex_);  // protect with stable delta sequnce.

                uint8_t *dest = discovery_packet_buffer_;
                memcpy(dest, &header_, sizeof(header_));
                dest += sizeof(header_);

                // This part is dynamic length.
                memcpy(dest, pixel_pusher_.base, pixel_pusher_base_size_);
                dest += pixel_pusher_base_size_;

                memcpy(dest, &pixel_pusher_.ext, sizeof(pixel_pusher_.ext));
                pixel_pusher_.base->delta_sequence = 0;
            }
            if (sendto(s, discovery_packet_buffer_, discovery_packet_size_, 0,
                       (struct sockaddr *) &addr, sizeof(addr)) < 0) {
                perror("Broadcasting problem");
            }
            nanosleep(&sleep_time, NULL);
        }
    }

private:
    const DiscoveryPacketHeader header_;
    PixelPusherContainer pixel_pusher_;
    const size_t pixel_pusher_base_size_;
    const size_t discovery_packet_size_;
    uint8_t *discovery_packet_buffer_;
    Mutex mutex_;
    uint32_t previous_sequence_;
};

class PacketReceiver : public StoppableThread {
public:
    PacketReceiver(FlaschenTaschen *c, Mutex *mutex,
                   Beacon *beacon) : matrix_(c), display_mutex_(mutex),
                                     beacon_(beacon) {
    }

    virtual void Run() {
        char *packet_buffer = new char[kMaxUDPPacketSize];
        const int strip_data_len = 1 /* strip number */ + 3 * matrix_->width();
        struct Pixel {
            uint8_t red;
            uint8_t green;
            uint8_t blue;
        };
        struct StripData {
            uint8_t strip_index;
            Pixel pixel[0];
        };

        int s;
        if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
            perror("creating listen socket");
            exit(1);
        }

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(kPixelPusherListenPort);
        if (bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
            perror("bind");
            exit(1);
        }
        fprintf(stderr, PXP "Listening for pixels pushed to port %d\n",
                kPixelPusherListenPort);
        while (running()) {
            ssize_t buffer_bytes = recvfrom(s, packet_buffer, kMaxUDPPacketSize,
                                            0, NULL, 0);
            if (!running())
                break;
            const int64_t start_time = CurrentTimeMicros();
            if (buffer_bytes < 0) {
                perror("receive problem");
                continue;
            }
            if (buffer_bytes <= 4) {
                fprintf(stderr, PXP "weird, no sequence number ? "
                        "Got %zd bytes\n", buffer_bytes);
            }

            // TODO: maybe continue when data same as before.

            const char *buf_pos = packet_buffer;

            uint32_t sequence;
            memcpy(&sequence, buf_pos, sizeof(sequence));
            buffer_bytes -= 4;
            buf_pos += 4;

            if (buffer_bytes % strip_data_len != 0) {
                fprintf(stderr, PXP "Expecting multiple of {1 + (rgb)*%d} = %d, "
                        "but got %zd bytes (leftover: %zd)\n", matrix_->width(),
                        strip_data_len, buffer_bytes,
                        buffer_bytes % strip_data_len);
                continue;
            }

            const int received_strips = buffer_bytes / strip_data_len;
            display_mutex_->Lock();
            for (int i = 0; i < received_strips; ++i) {
                StripData *data = (StripData *) buf_pos;
                // Copy into frame buffer.
                for (int x = 0; x < matrix_->width(); ++x) {
                    Color c(data->pixel[x].red,
                            data->pixel[x].green,
                            data->pixel[x].blue);
                    matrix_->SetPixel(x, data->strip_index, c);
                }
                buf_pos += strip_data_len;
            }
            matrix_->Send();
            display_mutex_->Unlock();

            const int64_t end_time = CurrentTimeMicros();
            beacon_->UpdatePacketStats(sequence, end_time - start_time);
        }
        delete [] packet_buffer;
    }

private:
    FlaschenTaschen *const matrix_;
    Mutex *const display_mutex_;
    Beacon *const beacon_;
};

static DiscoveryPacketHeader header;
static PixelPusherContainer pixel_pusher_container;
bool pixel_pusher_init(FlaschenTaschen *canvas) {
    const char *interface = kNetworkInterface;

    // Init PixelPusher protocol
    memset(&header, 0, sizeof(header));

    if (!DetermineNetwork(interface, &header)) {
        fprintf(stderr, PXP "Couldn't listen on network interface %s.\n",
                interface);
        return 1;
    }
    header.device_type = PIXELPUSHER;
    header.protocol_version = 1;  // ?
    header.vendor_id = 3;  // h.zeller@acm.org
    header.product_id = 0;
    header.sw_revision = kSoftwareRevision;
    header.link_speed = 10000000;  // 10MBit

    const int number_of_strips = canvas->height();
    const int pixels_per_strip = canvas->width();

    memset(&pixel_pusher_container, 0, sizeof(pixel_pusher_container));
    size_t base_size = CalcPixelPusherBaseSize(number_of_strips);
    pixel_pusher_container.base = (struct PixelPusherBase*) malloc(base_size);
    memset(pixel_pusher_container.base, 0, base_size);
    pixel_pusher_container.base->strips_attached = number_of_strips;
    pixel_pusher_container.base->pixels_per_strip = pixels_per_strip;
    static const int kUsablePacketSize = kMaxUDPPacketSize - 4; // 4 bytes seq#
    // Whatever fits in one packet, but not more than one 'frame'.
    pixel_pusher_container.base->max_strips_per_packet
        = std::min(kUsablePacketSize / (1 + 3 * pixels_per_strip),
                   number_of_strips);
    fprintf(stderr, PXP "Display: %dx%d (%d pixels each on %d strips)\n"
            "Accepting max %d strips per packet.\n",
            pixels_per_strip, number_of_strips,
            pixels_per_strip, number_of_strips,
            pixel_pusher_container.base->max_strips_per_packet);
    pixel_pusher_container.base->power_total = 1;         // ?
    pixel_pusher_container.base->update_period = 1000;   // initial assumption.
    pixel_pusher_container.base->controller_ordinal = 0;  // TODO: provide config
    pixel_pusher_container.base->group_ordinal = 0;       // TODO: provide config
    pixel_pusher_container.base->my_port = kPixelPusherListenPort;
    pixel_pusher_container.ext.pusher_flags = 0;
    pixel_pusher_container.ext.segments = 1;    // ?
    pixel_pusher_container.ext.power_domain = 0;
    return true;
}

void pixel_pusher_run_threads(FlaschenTaschen *display, Mutex *display_mutex) {
    // Create our threads.
    Beacon *discovery_beacon = new Beacon(header, pixel_pusher_container);
    PacketReceiver *receiver = new PacketReceiver(display, display_mutex,
                                                  discovery_beacon);

    receiver->Start(0);         // fairly low priority
    discovery_beacon->Start(5); // This should accurately send updates.
    fprintf(stderr, "Pixel pusher running %d.", kPixelPusherDiscoveryPort);
}
