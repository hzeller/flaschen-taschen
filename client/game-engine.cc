// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// Basic game framework
// Leonardo Romor <leonardo.romor@gmail.com>
// Henner Zeller <h.zeller@acm.org>
//

#include <arpa/inet.h>
#include <assert.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/time.h>
#include <unistd.h>

#include "udp-flaschen-taschen.h"
#include "game-engine.h"
#include "bdf-font.h"

// Size of the display. 9 x 7 crates.
#define DEFAULT_DISPLAY_WIDTH  (9 * 5)
#define DEFAULT_DISPLAY_HEIGHT (7 * 5)

#define FRAME_RATE 60

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

// An InputController receives inputs somehow and updats the input
// list.
// should be able to update the InputList (tbd, for now we accept a vector
// of 2 players each of them with an x,y (-1,1) "output")
class InputController {
public:
    ~InputController() {}
    virtual void UpdateInputList(Game::InputList *inputs_list,
                                 uint64_t timeout_us) = 0;
};

// Limited to 2 players (easily extendible)
class UDPInputServer : public InputController {
public:
    // Init the UDP server and the Joystick logic
    UDPInputServer(uint16_t port);
    ~UDPInputServer() { if (fd_ > 0) close(fd_); }

    void UpdateInputList(Game::InputList *inputs_list, uint64_t timeout_us);
    void RegisterPlayers(UDPFlaschenTaschen *display,
                         const ft::Font &font);

private:
    UDPInputServer() {}
    uint16_t p1port_, p2port_; // Maybe struct and typedef?
    char p1addr_[INET6_ADDRSTRLEN];
    char p2addr_[INET6_ADDRSTRLEN];
    int fd_;
};

UDPInputServer::UDPInputServer(uint16_t port) {
    if ((fd_ = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("IPv6 in kernel enabled ? While creating listen socket.");
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

void UDPInputServer::RegisterPlayers(UDPFlaschenTaschen *display,
                                     const ft::Font &font) {
    struct sockaddr_in6 client_address;
    unsigned int address_length;
    address_length = sizeof(struct sockaddr);

    const Color text_bg(0, 0, 1);
    ft::DrawText(display, font, 0, 4, Color(255, 0, 0), &text_bg,
                 "1. Player");
    display->Send();

    recvfrom(fd_, NULL, 0,
             0, (struct sockaddr *) &client_address,
             &address_length);
    p1port_ = ntohs(client_address.sin6_port);
    inet_ntop(AF_INET6, &client_address, (char *)&p1addr_, INET6_ADDRSTRLEN);
    fprintf(stdout, "Registered p1: %s:%u\n", p1addr_, p1port_);

    ft::DrawText(display, font, 0, 4, Color(0, 255, 0), &text_bg,
                 "1. OK    ");
    ft::DrawText(display, font, 0, 14, Color(255, 0, 0), &text_bg,
                 "2. Player");
    display->Send();
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
    ft::DrawText(display, font, 0, 14, Color(0, 255, 0), &text_bg,
                 "2. OK    ");
}

void UDPInputServer::UpdateInputList(Game::InputList *inputs_list,
                                     uint64_t timeout_us) {
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

static int usage(const char *progname) {
    fprintf(stderr, "usage: %s [options]\n", progname);
    fprintf(stderr, "Options:\n"
            "\t-g <width>x<height>[+<off_x>+<off_y>[+<layer>]] : Output geometry. Default 45x35\n"
            "\t-l <layer>      : Layer 0..15. Default 0 (note if also given in -g, then last counts)\n"
            "\t-h <host>       : Flaschen-Taschen display hostname.\n"
            "\t-p <port>       : Game input port.\n"
            "\t-b <RRGGBB>     : Background color as hex (default: 000000)\n"
            );

    return 1;
}

int RunGame(const int argc, char *argv[], Game *game) {
    const char *hostname = NULL;   // Will use default if not set otherwise.
    int width = DEFAULT_DISPLAY_WIDTH;
    int height= DEFAULT_DISPLAY_HEIGHT;
    int off_x = 0;
    int off_y = 0;
    int off_z = 0;
    int remote_port = 4321;
    Color background(0, 0, 0);

    if (argc < 2) {
        fprintf(stderr, "Mandatory argument(s) missing\n");
        return usage(argv[0]);
    }

    int opt;
    while ((opt = getopt(argc, argv, "g:l:h:p:b:")) != -1) {
        switch (opt) {
        case 'g':
            if (sscanf(optarg, "%dx%d%d%d%d", &width, &height, &off_x, &off_y, &off_z)
                < 2) {
                fprintf(stderr, "Invalid size spec '%s'", optarg);
                return usage(argv[0]);
            }
            break;
        case 'p':
            remote_port = atoi(optarg);
            if (remote_port < 1023 || remote_port > 65535) {
                fprintf(stderr, "Invalid port '%s'\n", optarg);
                return usage(argv[0]);
            }
            break;
        case 'h':
            hostname = strdup(optarg); // leaking. Ignore.
            break;
        case 'l':
            if (sscanf(optarg, "%d", &off_z) != 1 || off_z < 0 || off_z >= 16) {
                fprintf(stderr, "Invalid layer '%s'\n", optarg);
                return usage(argv[0]);
            }
            break;
        case 'b': {
            int r, g, b;
            if (sscanf(optarg, "%02x%02x%02x", &r, &g, &b) != 3) {
                fprintf(stderr, "Background color parse error\n");
                return usage(argv[0]);
            }
            background.r = r; background.g = g; background.b = b;
            break;
        }
        default:
            return usage(argv[0]);
        }
    }

    ft::Font font;
    font.LoadFont("fonts/5x5.bdf");  // TODO: don't hardcode.

    const int display_socket = OpenFlaschenTaschenSocket(hostname);
    UDPFlaschenTaschen display(display_socket, width, height);
    display.SetOffset(off_x, off_y, off_z);

    UDPInputServer inputs(remote_port);
    inputs.RegisterPlayers(&display, font);

    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    std::vector<GameInput> inputs_list;
    // Fill the vector with 2 player inputs
    inputs_list.push_back(GameInput());
    inputs_list.push_back(GameInput());

    Color black;
    for (int countdown = 3; countdown > 0; countdown--) {
        char buffer[10];
        snprintf(buffer, sizeof(buffer), "%d%*s", countdown, 4-countdown, ">");
        ft::DrawText(&display, font, 0, 24, Color(255, 255, 0), &black, buffer);
        display.Send();
        sleep(1);
    }

    fprintf(stdout, "Start game.\n");

    display.Clear();
    display.Send();

    game->SetCanvas(&display, background);

    const uint64_t frame_interval = 1000000 / FRAME_RATE;
    struct timeval tp;
    gettimeofday(&tp, NULL);
    const int64_t start_time = tp.tv_sec * 1000000 + tp.tv_usec;

    while (!interrupt_received) {
        inputs.UpdateInputList(&inputs_list, frame_interval);
        gettimeofday(&tp, NULL);
        const int64_t now = tp.tv_sec * 1000000 + tp.tv_usec;
        game->UpdateFrame(now - start_time, inputs_list);
    }

    display.Clear();
    display.Send();

    fprintf(stderr, "Good bye.\n");
    return 0;
}
