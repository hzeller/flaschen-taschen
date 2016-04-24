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
#include <iostream>
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

class JoyStick {
public:
    class EventListener {
    public:
        virtual void button_pressed(const int button, const bool status) = 0;
        virtual void axis_moving(const int axis, const int value) = 0;
    };

    JoyStick(const char *js, EventListener &handler);
    ~JoyStick() { close(fd_); }
    void Start();
private:
    const int fd_;
    EventListener &handler_;
    JoyStick();
};
//Need select
//read (fd, &e, sizeof(e));
JoyStick::JoyStick(const char *js, EventListener &handler) : fd_(open(js, O_RDONLY)), handler_(handler){
    if(fd_ < 0) {
        std::cerr << "Error opening joystick" << std::endl;
        exit(0);
    }
}

void JoyStick::Start() {
    // Select stuff
    js_event event;
    fd_set rfds;
    struct timeval tv;
    int retval;

    for (;;) {
        FD_ZERO(&rfds);
        FD_SET(this->fd_, &rfds);
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        retval = select(fd_ + 1, &rfds, NULL, NULL, &tv);
        if (retval == -1)
            std::cerr << "Error on select" << std::endl;
        else if (retval) {
            read(fd_, &event, sizeof(event));
            switch (event.type) {
            case JS_EVENT_BUTTON:
                handler_.button_pressed(event.number,event.value);
                break;
            case JS_EVENT_AXIS:
                handler_.axis_moving(event.number,(bool)event.value);
                break;
            }
        }
    }
}

class EventHandler : public JoyStick::EventListener {
public:
    virtual void button_pressed(const int button, const bool status);
    virtual void axis_moving(const int axis, const int value);
};

void EventHandler::button_pressed(const int button, const bool status){
    std::cout << "button: " << button << " status: " << status << std::endl;
}

void EventHandler::axis_moving(const int axis, const int value){
    std::cout << "axis: " << axis << " value: " << value << std::endl;
}

int main(int argc, char *argv[]){
    const char *js_file = "/dev/input/js0";
    if ( argc > 1 ) {
        js_file = argv[1];
    }

    EventHandler event_handler = EventHandler();
    JoyStick js(js_file, event_handler);
    js.Start();

    return 0;
}
