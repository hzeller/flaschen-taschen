// -*- mode: cc; c-basic-offset: 4; indent-tabs-mode: nil; -*-

#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <pthread.h>

#include "servers.h"

// public interface
static int server_socket = -1;
bool udp_server_init(int port) {
    if ((server_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("creating listen socket");
        return false;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    if (bind(server_socket, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("bind");
        return false;
    }

    fprintf(stderr, "UDP ready to listen on %d\n", port);
    return true;
}

void udp_server_run(FlaschenTaschen *display, pthread_mutex_t *mutex) {
    int frame_buffer_size = display->width() * display->height() * 3;
    uint8_t *packet_buffer = new uint8_t[frame_buffer_size];
    bzero(packet_buffer, frame_buffer_size);

    for (;;) {
        ssize_t received_bytes = recvfrom(server_socket,
                                          packet_buffer, frame_buffer_size,
                                          0, NULL, 0);
        if (received_bytes < 0) {
            perror("Trouble receiving.");
            break;
        }

        pthread_mutex_lock(mutex);
        uint8_t *pixel_pos = packet_buffer;
        for (int y = 0; y < display->height(); ++y) {
            for (int x = 0; x < display->width(); ++x) {
                Color c;
                c.r = *pixel_pos++;
                c.g = *pixel_pos++;
                c.b = *pixel_pos++;
                display->SetPixel(x, y, c);
            }
        }
        display->Send();
        pthread_mutex_unlock(mutex);
    }
}
