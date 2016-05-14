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

#include <linux/joystick.h>
#include <netdb.h>
#include <stdio.h>
#include <limits.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>

#include "timer.h"

#define JS_EVENT_BUTTON 0x01
#define JS_EVENT_AXIS 0x02
#define JS_EVENT_INIT 0x80

#define VFACTOR -1e-3f // Speed factor, for now hardcoded, -1 to reverse axis dir,
                       // it UpdatePosply rescale the maximum speed.
#define YAXIS 1 // Joystick left Y axis
#define XAXIS 0 // Joystick left X axis

#define KEYBOARD_STEP (SHRT_MAX - 1) / 8 // Range from -32767 .. +32767

struct ClientOutput {
    int16_t x_pos;  // Range -32767 .. +32767
    int16_t y_pos;  // Range -32767 .. +32767
    // buttons...
};

int OpenClientSocket(const char *host, const char *port) {
    if (host == NULL || port == NULL) {
        fprintf(stderr, "Error, no host or port specified\n");
        return -1;
    }
    struct addrinfo addr_hints = {0};
    addr_hints.ai_family = AF_INET;
    addr_hints.ai_socktype = SOCK_DGRAM;

    struct addrinfo *addr_result = NULL;
    int rc;
    if ((rc = getaddrinfo(host, port, &addr_hints, &addr_result)) != 0) {
        fprintf(stderr, "Resolving '%s': %s\n", host, gai_strerror(rc));
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

class Controller {
public:
    virtual ~Controller() {}
    virtual void WaitEvent(ClientOutput *output, unsigned int us = 1e6) = 0;
};

class JoyStick : public Controller {
public:
    JoyStick(const char *js);
    ~JoyStick() { close(fd_); }
    void WaitEvent(ClientOutput *output, unsigned int us = 1e6);
private:
    const int fd_;
    JoyStick();
};

JoyStick::JoyStick(const char *js) : fd_(open(js, O_RDONLY)) {
    if (fd_ < 0) {
        fprintf(stderr, "Error opening joystick");
        exit(1);
    }
}

void JoyStick::WaitEvent(ClientOutput *output, unsigned int us) {
    // Select stuff
    js_event event;
    fd_set rfds;
    struct timeval tv;
    int retval;
    FD_ZERO(&rfds);
    FD_SET(this->fd_, &rfds);
    tv.tv_sec = 0;
    tv.tv_usec = us;
    retval = select(fd_ + 1, &rfds, NULL, NULL, &tv);
    if (retval == -1)
        fprintf(stderr, "Error on select");
    else if (retval) {
        read(fd_, &event, sizeof(event));
        switch (event.type) {
        case JS_EVENT_BUTTON:
            break;
        case JS_EVENT_AXIS:
            // Horrible (split?)
            switch (event.number) {
            case XAXIS:
                output->x_pos = event.value;
                break;
            case YAXIS:
                output->y_pos = event.value;
                break;
            }
            break;
        }
    }
}

struct termios orig_termios;
static void reset_terminal_mode() {
    tcsetattr(0, TCSANOW, &orig_termios);
}

static void set_conio_terminal_mode() {
    struct termios new_termios;

    /* take two copies - one for now, one for later */
    tcgetattr(0, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));

    /* register cleanup handler, and set the new terminal mode */
    atexit(reset_terminal_mode);
    cfmakeraw(&new_termios);
    tcsetattr(0, TCSANOW, &new_termios);
}

class Keyboard : public Controller {
public:
    Keyboard();
    void WaitEvent(ClientOutput *output, unsigned int us = 1e6);
private:
    int16_t ypos_;
};

Keyboard::Keyboard() : ypos_(0) {
    // Keyboard mode
    fprintf(stdout, "Game keys:\n"
           "Left paddel:  'w' up; 'd' down\n"
           "Quit with 'q'.\n");
    set_conio_terminal_mode();
}

void Keyboard::WaitEvent(ClientOutput *output, unsigned int us) {
    int fd = 0;
    char command;
    int32_t limit = ypos_;
    // Select stuff
    fd_set rfds;
    struct timeval tv;
    int retval;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    tv.tv_sec = 0;
    tv.tv_usec = us;
    retval = select(fd + 1, &rfds, NULL, NULL, &tv);

    if (retval < 0)
        fprintf(stderr, "Error on select\n");
    else if (retval && read(0, &command, sizeof(char)) == 1) {
        switch (command) {
        case 'd': case 'D':
            limit += KEYBOARD_STEP;
            if (limit > SHRT_MAX - 1) ypos_ = SHRT_MAX - 1;
            else ypos_ += KEYBOARD_STEP;
            break;
        case 'w': case 'W':
            limit -= KEYBOARD_STEP;
            if (limit < -SHRT_MAX + 1) ypos_ = -SHRT_MAX + 1;
            else ypos_ -= KEYBOARD_STEP;
            break;
        case 3:   // In raw mode, we receive Ctrl-C as raw byte.
        case 'q':
            reset_terminal_mode();
            exit(0);
        }
    }
    output->y_pos = ypos_;
}

// For now just transmitter (we could add vibration if supported when a player lose or hits the ball)
class UDPGameClient {
public:
    UDPGameClient(const char *host, const char *port);
    ~UDPGameClient() { close(udp_fd_); }
    void Send(ClientOutput input);
private:
    const int udp_fd_;
};

UDPGameClient::UDPGameClient(const char *host, const char *port) :
    udp_fd_(OpenClientSocket(host, port)) {
    if (udp_fd_ < 0) {
        fprintf(stderr, "Error on opening udp socket\n");
        exit(1);
    }
}

inline void htonsarray(uint16_t *data, int len) {
    for (int i = 0; i < len; ++i) {
        data[i] = htons(data[i]);
    }
}

void UDPGameClient::Send(ClientOutput output) {
    output.x_pos = htons(output.x_pos);
    output.y_pos = htons(output.y_pos);
    if (write(udp_fd_, &output, sizeof(ClientOutput)) < 0) return;
}

static int usage(const char *progname) {
    fprintf(stderr, "usage: %s [options]\n", progname);
    fprintf(stderr, "Options:\n"
            "\t-j <dev>        : Joypad device file\n"
            "\t-h <host>       : Flaschen-Taschen display hostname.\n"
            "\t-p <port>       : remote operation: listen on port\n"
            );

    return 1;
}

int main(int argc, char *argv[]) {

    const char *hostname = NULL;
    const char *remote_port = "4321";
    int port;
    Controller *control = NULL;

    if (argc < 2) {
        fprintf(stderr, "Mandatory argument(s) missing\n");
        return usage(argv[0]);
    }

    int opt;
    while ((opt = getopt(argc, argv, "j:h:p:")) != -1) {
        switch (opt) {
        case 'p':
            port = atoi(optarg);
            if (port < 1023 || port > 65535) {
                fprintf(stderr, "Invalid port '%d'\n", atoi(optarg));
                return usage(argv[0]);
            }
            remote_port = strdup(optarg);
            break;
        case 'h':
            hostname = strdup(optarg);
            break;
        case 'j':
            control = new JoyStick(strdup(optarg));
            break;
        default:
            return usage(argv[0]);
        }
    }

    UDPGameClient client = UDPGameClient(hostname, remote_port);
    if (!control) control = new Keyboard();
    ClientOutput output;
    for (;;) {
        control->WaitEvent(&output);
        client.Send(output);
    }

    delete control;
    return 0;
}
