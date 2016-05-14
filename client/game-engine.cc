// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// Basic game framework
// Leonardo Romor <leonardo.romor@gmail.com>
// Henner Zeller <h.zeller@acm.org>
//

#include <stdio.h>
#include <sys/time.h>
#include <signal.h>
#include <arpa/inet.h>
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <unistd.h>

#include "udp-flaschen-taschen.h"
#include "game-engine.h"
#include "timer.h"

// Share it with a .h?
struct ClientOutput {
    int16_t x_pos;  // Range -32767 .. +32767
    int16_t y_pos;  // Range -32767 .. +32767
    // buttons...
};

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

// Limited to 2 players (easily extendible)
class UDPInputServer : public InputController {
public:
    // Init the UDP server and the Joystick logic
    UDPInputServer(uint16_t port);
    ~UDPInputServer() { if (fd_ > 0) close(fd_); }

    void UpdateInputList(InputList *inputs_list, uint64_t timeout_us);
    void RegisterPlayers();
private:
    UDPInputServer() {}
    uint16_t p1port_, p2port_; // Maybe struct and typedef?
    unsigned char p1addr_[INET6_ADDRSTRLEN], p2addr_[INET6_ADDRSTRLEN];
    int fd_;
};

UDPInputServer::UDPInputServer(uint16_t port) {
    if ((fd_ = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("IPv6 enabled ? While reating listen socket");
        exit(1);
    }
    int opt = 0;   // Unset IPv6-only, in case it is set. Best effort.
    setsockopt(fd_, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));

    // Set a non blocking socket?
    int flags = fcntl(fd_, F_GETFL);
    // flags |= O_NONBLOCK;
    fcntl(fd_, F_SETFL, flags);

    struct sockaddr_in6 addr = {0};
    addr.sin6_family = AF_INET6;
    addr.sin6_addr = in6addr_any;
    addr.sin6_port = htons(port);
    if (bind(fd_, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("Error on bind");
        exit(1);
    }
    fprintf(stdout, "UDPInputServer started on port: %u\n", port);
}

void UDPInputServer::RegisterPlayers() {
    struct sockaddr_in6 client_address;
    unsigned int address_length;
    address_length = sizeof(struct sockaddr);

    fprintf(stdout, "Waiting for first player...(Move something)\n");
    recvfrom(fd_, NULL, 0,
                0, (struct sockaddr *) &client_address,
                &address_length);
    p1port_ = ntohs(client_address.sin6_port);
    inet_ntop(AF_INET6, &client_address, (char *)&p1addr_, INET6_ADDRSTRLEN);
    fprintf(stdout, "Registered p1: %s:%u\n", p1addr_, p1port_);

    fprintf(stdout, "Waiting for second player...(Move something)\n");
    for (;;) {
        recvfrom(fd_, NULL, 0,
                                0, (struct sockaddr *) &client_address,
                                &address_length);
        p2port_ = ntohs(client_address.sin6_port);
        inet_ntop(AF_INET6, &client_address, (char *) &p2addr_, INET6_ADDRSTRLEN);
        if (p2port_ != p1port_) {
            fprintf(stdout, "Registered p2: %s:%u\n", p2addr_, p2port_);
            break;
        }
    }
}

void UDPInputServer::UpdateInputList(InputList *inputs_list, uint64_t timeout_us) {
    ClientOutput data_received;
    struct sockaddr_in6 client_address;
    unsigned int address_length;
    int retval = 0, bytes_read = 0;
    address_length = sizeof(struct sockaddr);

    // Select variables
    fd_set rfds;
    struct timeval tv;

    FD_ZERO(&rfds);
    FD_SET(fd_, &rfds);
    tv.tv_sec = 0;
    tv.tv_usec = timeout_us;

    retval = select(fd_ + 1, &rfds, NULL, NULL, &tv);
    if (retval > 0) {
        bytes_read = recvfrom(fd_, &data_received, sizeof(ClientOutput),
                              0, (struct sockaddr *) &client_address,
                              &address_length);
        if (bytes_read == sizeof(ClientOutput)) {
            if (ntohs(client_address.sin6_port) == p2port_) {
                (*inputs_list)[1].x_pos = (int16_t) ntohs(data_received.x_pos) / (float) SHRT_MAX;
                (*inputs_list)[1].y_pos = (int16_t) ntohs(data_received.y_pos) / (float) SHRT_MAX;
            } else if (ntohs(client_address.sin6_port) == p1port_) {
                (*inputs_list)[0].x_pos = (int16_t) ntohs(data_received.x_pos) / (float) SHRT_MAX;
                (*inputs_list)[0].y_pos = (int16_t) ntohs(data_received.y_pos) / (float) SHRT_MAX;
            }
        }
    }
}

void RunGame(Game *game, int frame_rate, int remote_port) {
    if (remote_port < 1023 || remote_port > 65535) {
        fprintf(stderr, "Invalid port specified\n");
        exit(1);
    }

    UDPInputServer inputs(remote_port);
    inputs.RegisterPlayers();

    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    std::vector<GameInput> inputs_list;
    // Fill the vector with 2 player inputs
    inputs_list.push_back(GameInput());
    inputs_list.push_back(GameInput());

    fprintf(stdout, "Starting the game...\n");
    Timer t;
    // Start time
    t.Start();
    while (!interrupt_received) {
        inputs.UpdateInputList(&inputs_list, 1e6 / (float) frame_rate);
        game->UpdateFrame(t.GetElapsedInMicroseconds(), inputs_list);
        // Start time, we need the time between already sent frames.
        t.Start();
    }
}
