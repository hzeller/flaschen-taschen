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

//Speed factor for now hardcoded
#define VFACTOR 1e5

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

int UDPClient(const char *host, const char *port) {
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
        virtual void button_pressed(const int button, const int status) = 0;
        virtual void axis_moving(const int axis, const int value) = 0;
    };

    JoyStick(const char *js, EventListener &handler);
    ~JoyStick() { close(fd_); }
    void WaitEvent( const float ms = 1);
private:
    const int fd_;
    EventListener &handler_;
    JoyStick();
};

JoyStick::JoyStick(const char *js, EventListener &handler) : fd_(open(js, O_RDONLY)), handler_(handler){
    if (fd_ < 0) {
        std::cerr << "Error opening joystick" << std::endl;
        exit(0);
    }
}

void JoyStick::WaitEvent(const float ms) {
    assert(ms >= 0);
    const unsigned int us = ms * 1000;
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
            handler_.button_pressed(event.number, event.value);
            break;
        case JS_EVENT_AXIS:
            handler_.axis_moving(event.number, event.value);
            break;
        }
    }
}

//For now just transmitter (we could add vibration if supported when a player lose)
class PongClient : public JoyStick::EventListener {
public:
    PongClient(const char *host, const char *port);
    void Sim(const float dt);
    void Send();
    virtual void button_pressed(const int button, const int status) {}
    virtual void axis_moving(const int axis, const int value);

private:
    const int udp_fd_;
    int16_t pos_;
    int16_t speed_;
};

PongClient::PongClient(const char *host, const char *port) :
    udp_fd_(UDPClient(host, port)), pos_(0) {
    if (udp_fd_ < 0) {
        std::cerr << "Error on opening udp socket" << std::endl;
        exit(0);
    }
}

void PongClient::axis_moving(const int axis, const int value){
    if(axis == 1){
        speed_ = value / VFACTOR;
    }
}

void PongClient::Sim(const float dt) {
    pos_ += speed_ * dt;
}

void PongClient::Send() {
    if (write(udp_fd_, &pos_, sizeof(pos_)) < 0) return;
}

int main(int argc, char *argv[]){
    const char *js_file = "/dev/input/js0";
    if ( argc > 1 ) {
        js_file = argv[1];
    }

    PongClient pong_client = PongClient("localhost", "10000");
    JoyStick js(js_file, pong_client);
    Timer t;
    float dt = 0;
    for (;;) {
        t.Start();
        js.WaitEvent();
        pong_client.Sim(dt);
        pong_client.Send();
        dt = t.GetElapsedInMilliseconds();
    }

    return 0;
}
