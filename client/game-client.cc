// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// Leonardo Romor <leonardo.romor@gmail.com>
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
#include <signal.h>

#include "game-engine-private.h"

#define JS_EVENT_BUTTON 0x01
#define JS_EVENT_AXIS 0x02
#define JS_EVENT_INIT 0x80

#define JS_YAXIS 1 // Joystick left Y axis
#define JS_XAXIS 0 // Joystick left X axis

#define KEYBOARD_STEP (SHRT_MAX - 1) / 8 // Range from -32767 .. +32767

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
    interrupt_received = true;
}

static uint64_t NowUsec() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return tp.tv_sec * 1000000LL + tp.tv_usec;
}

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
    enum EventResult {
        EV_UPDATED_POS = 1, // updated position.
        EV_TIMEOUT = 2,     // timeout
        EV_FINISHED = 3,    // exit.
    };
    virtual ~Controller() {}
    virtual EventResult WaitEvent(ClientOutput *output, int usec) = 0;
};

class JoyStick : public Controller {
public:
    JoyStick(const char *js);
    ~JoyStick() { close(fd_); }
    virtual EventResult WaitEvent(ClientOutput *output, int usec);

private:
    const int fd_;
    JoyStick();
};

JoyStick::JoyStick(const char *js) : fd_(open(js, O_RDONLY)) {
    if (fd_ < 0) {
        perror("Error opening joystick");
        exit(1);
    }
}

Controller::EventResult JoyStick::WaitEvent(ClientOutput *output, int usec) {
    const int64_t kMaxEventSpeed = 1000000 / 80; // max 80Hz
    // Select stuff
    js_event event;
    fd_set rfds;
    struct timeval tv;
    int retval;
    tv.tv_sec = usec / 1000000;
    tv.tv_usec = usec % 1000000;
    EventResult result = EV_TIMEOUT;
    const int64_t start_time = NowUsec();
    while (tv.tv_sec > 0 || tv.tv_usec > 0) {
        // TODO: currently relying on Linux feature to update tv.
        FD_ZERO(&rfds);
        FD_SET(this->fd_, &rfds);
        retval = select(fd_ + 1, &rfds, NULL, NULL, &tv);
        if (retval == 0) {
            return EV_TIMEOUT;
        } else if (retval == -1) {
            perror("Error on select");
            return EV_FINISHED;
        } else if (read(fd_, &event, sizeof(event)) == sizeof(event)) {
            const int64_t event_time = NowUsec() - start_time;
            const bool too_fast = event_time < kMaxEventSpeed;
            switch (event.type) {
            case JS_EVENT_BUTTON:
                break;
            case JS_EVENT_AXIS:
                // Horrible (split?)
                switch (event.number) {
                case JS_XAXIS:
                    if (output->x_pos != event.value) {
                        output->x_pos = event.value;
                        result = EV_UPDATED_POS;
                        if (!too_fast) return result;
                        // So we got an event, but don't want to send these
                        // too fast. If we got a result, but too quickly, we
                        // wait until the time is up. There might be some other
                        // event coming in that time.
                        tv.tv_sec = 0; tv.tv_usec = kMaxEventSpeed - event_time;
                    }
                    break;
                case JS_YAXIS:
                    if (output->y_pos != event.value) {
                        output->y_pos = event.value;
                        result = EV_UPDATED_POS;
                        if (!too_fast) return result;
                        tv.tv_sec = 0; tv.tv_usec = kMaxEventSpeed - event_time;
                    }
                    break;
                }
            }
        } else {
            return EV_FINISHED;
        }
    }
    return result;
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
    virtual EventResult WaitEvent(ClientOutput *output, int usec);

private:
    int xpos_;
    int ypos_;
};

Keyboard::Keyboard() : xpos_(0), ypos_(0) {
    // Keyboard mode
    fprintf(stdout, "Game keys: standard vi-keys on keyboard:\n"
            "'H'=Left | 'J'=Down | 'K'=Up  | 'L'=Right\n"
            "K     ^     K\n"
            "H <-    ->  L\n"
            "J     V     J\n"
            "(q)uit\n");
    set_conio_terminal_mode();
}

Controller::EventResult Keyboard::WaitEvent(ClientOutput *output, int usec) {
    int fd = STDIN_FILENO;
    char command;
    // Select stuff
    fd_set rfds;
    struct timeval tv;
    int retval;
    tv.tv_sec = usec / 1000000;
    tv.tv_usec = usec % 1000000;

    while (tv.tv_sec > 0 || tv.tv_usec > 0) {
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        retval = select(fd + 1, &rfds, NULL, NULL, &tv);
        if (retval == 0) {
            return EV_TIMEOUT;
        } else if (retval < 0) {
            perror("Error on select\n");
            return EV_FINISHED;
        } else if (read(0, &command, sizeof(char)) == sizeof(char)) {
            switch (command) {
            case 'd': case 'D': // Legacy
            case 'j': case 'J': // vim binding.
                ypos_ += KEYBOARD_STEP;
                if (ypos_ > SHRT_MAX - 1) ypos_ = SHRT_MAX - 1;
                output->y_pos = ypos_;
                return EV_UPDATED_POS;

            case 'w': case 'W':  // Legacy
            case 'k': case 'K':  // vim binding.
                ypos_ -= KEYBOARD_STEP;
                if (ypos_ < -SHRT_MAX + 1) ypos_ = -SHRT_MAX + 1;
                output->y_pos = ypos_;
                return EV_UPDATED_POS;

            case 'l': case 'L':
                xpos_ += KEYBOARD_STEP;
                if (xpos_ > SHRT_MAX - 1) xpos_ = SHRT_MAX - 1;
                output->x_pos = xpos_;
                return EV_UPDATED_POS;

            case 'h': case 'H':
                xpos_ -= KEYBOARD_STEP;
                if (xpos_ < -SHRT_MAX + 1) xpos_ = -SHRT_MAX + 1;
                output->x_pos = xpos_;
                return EV_UPDATED_POS;

            case 3:   // In raw mode, we receive Ctrl-C as raw byte.
            case 'q':
                reset_terminal_mode();
                return EV_FINISHED;
            }
        }
    }
    return EV_TIMEOUT;
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
            "\t-j <dev>        : Joypad device file (e.g. /dev/input/js0)\n"
            "\t-h <host>       : Game hostname.\n"
            "\t-p <port>       : Remote game port (default: 4321)\n"
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
            control = new JoyStick(optarg);
            break;
        default:
            return usage(argv[0]);
        }
    }

    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    UDPGameClient client = UDPGameClient(hostname, remote_port);
    if (!control) control = new Keyboard();
    ClientOutput output;
    const int kTimeout = 1000000 / 2;  // Ping at least twice a second.
    while (!interrupt_received
           && control->WaitEvent(&output, kTimeout) != Controller::EV_FINISHED) {
        client.Send(output);
    }

    delete control;
    fprintf(stderr, "Exiting.\n");

    // Make sure the exit message is not lost.
    output.b.but_exit = 1;
    for (int i = 0; i < 3; ++i) { client.Send(output); }

    return 0;
}
