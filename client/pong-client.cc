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
#include <assert.h>
#include <iostream>
#include <netdb.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>

#define JS_EVENT_BUTTON 0x01
#define JS_EVENT_AXIS 0x02
#define JS_EVENT_INIT 0x80

#define VFACTOR -1e-3f // Speed factor, for now hardcoded, -1 to reverse axis dir,
                       // it UpdatePosply rescale the maximum speed.
#define AXIS 1 // Joystick pong axis

#define SHRT_MAX 32768

class Timer {
public:
    Timer() : start_(0) {}

    void Start() {
        struct timeval tp;
        gettimeofday(&tp, NULL);
        start_ = tp.tv_sec * 1000000 + tp.tv_usec;
    }

    float GetElapsedInMilliseconds() {
        struct timeval tp;
        gettimeofday(&tp, NULL);
        return (tp.tv_sec * 1000000 + tp.tv_usec - start_) / 1e3;
    }

    inline void clear() { start_ = 0; }

private:
    int64_t start_;
};

int OpenClientSocket(const char *host, const char *port) {
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

class JoyStick {
public:
    class EventListener {
    public:
        virtual ~EventListener() {}
        virtual void button_pressed(int button, int status) = 0;
        virtual void axis_moving(int axis, int value) = 0;
    };

    JoyStick(const char *js, EventListener *handler);
    ~JoyStick() { close(fd_); }
    void WaitEvent(unsigned int us = 1000);
private:
    const int fd_;
    EventListener *handler_;
    JoyStick();
};

JoyStick::JoyStick(const char *js, EventListener *handler) : fd_(open(js, O_RDONLY)), handler_(handler){
    if (fd_ < 0) {
        std::cerr << "Error opening joystick" << std::endl;
        exit(1);
    }
}

void JoyStick::WaitEvent(unsigned int us) {
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
        std::cerr << "Error on select" << std::endl;
    else if (retval) {
        read(fd_, &event, sizeof(event));
        switch (event.type) {
        case JS_EVENT_BUTTON:
            handler_->button_pressed(event.number, event.value);
            break;
        case JS_EVENT_AXIS:
            handler_->axis_moving(event.number, event.value);
            break;
        }
    }
}

// For now just transmitter (we could add vibration if supported when a player lose or hits the ball)
class PongClient : public JoyStick::EventListener {
public:
    PongClient(const char *host, const char *port);
    bool UpdatePos(float dt);
    void Send();
    float GetPos();
    virtual void button_pressed(int button, int status) {}
    virtual void axis_moving(int axis, int value);
private:
    const int udp_fd_;
    int16_t pos_;
    float speed_;
};

PongClient::PongClient(const char *host, const char *port) :
    udp_fd_(OpenClientSocket(host, port)), pos_(0), speed_(0) {
    if (udp_fd_ < 0) {
        std::cerr << "Error on opening udp socket" << std::endl;
        exit(1);
    }
}

float PongClient::GetPos() {
    return pos_ / (float)SHRT_MAX; // Return the position between -1 and 1, useful to debug
}

void PongClient::axis_moving(int axis, int value) {
    if (axis == AXIS) {
        speed_ = value * VFACTOR;
    }
}

bool PongClient::UpdatePos(float dt) {
    if (speed_ == 0) return false;
    int32_t new_pos = pos_ + speed_ * dt;
    if (new_pos > SHRT_MAX - 1) {
        new_pos = SHRT_MAX - 1;
    } else if (new_pos < - SHRT_MAX) {
        new_pos = - SHRT_MAX;
    }
    pos_ = new_pos;
    return true;
}

void PongClient::Send() {
    uint16_t to_send = htons(pos_);
    if (write(udp_fd_, &to_send, sizeof(to_send)) < 0) return;
}

int main(int argc, char *argv[]){
    const char *js_file = "/dev/input/js0";
    if (argc > 1) {
        js_file = argv[1];
    }

    PongClient pong_client = PongClient("localhost", "10000");
    JoyStick js(js_file, &pong_client);
    Timer t;
    float dt = 0;
    t.Start();
    for (;;) {
        js.WaitEvent();
        dt = t.GetElapsedInMilliseconds();
        t.Start();
        if (pong_client.UpdatePos(dt)) pong_client.Send();
        std::cout << pong_client.GetPos() << std::endl;
    }

    return 0;
}
