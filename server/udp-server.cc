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
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
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
#include "ppm-reader.h"

#include <Magick++.h>

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
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

    struct sigaction sa = {{0}};  // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53119
    sa.sa_handler = InterruptHandler;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    // Initialise ImageMagick library
    Magick::InitializeMagick("");

    for (;;) {
        // TODO: also store src-address in case we want to do rate-limiting
        // per source-address.
        ssize_t received_bytes = recvfrom(server_socket,
                                          packet_buffer, kBufferSize,
                                          0, NULL, 0);
        if (interrupt_received)
            break;

        if (received_bytes < 0 && errno == EINTR) // Other signals. Don't care.
            continue;

        if (received_bytes < 0) {
            perror("Trouble receiving.");
            break;
        }

        ImageMetaInfo img_info = {0};
        img_info.width = display->width();  // defaults.
        img_info.height = display->height();

        // Create Image object and read in from pixel data above
        const char *pixel_pos = ReadImageData(packet_buffer, received_bytes,
                                           &img_info);

        // pointer needed for image magick
        char* magick_ptr = NULL;

        if (img_info.type == ImageType::Q7) {
            magick_ptr = new char[3 * img_info.width * img_info.height];

            // convert to bmp with GraphicsMagick Lib
            Magick::Blob inblob((void*)pixel_pos, received_bytes);
            Magick::Image image(inblob);

            image.write(0, 0, img_info.width, img_info.height, "RGB", Magick::CharPixel, (void*)magick_ptr);
            pixel_pos = magick_ptr;
        }

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

        delete [] magick_ptr;
    }
    delete [] packet_buffer;
}
