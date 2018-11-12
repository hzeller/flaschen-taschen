// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
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
//
// Various servers we support, all competing for the same display.

#ifndef FT_SERVER_H
#define FT_SERVER_H

class FlaschenTaschen;
class CompositeFlaschenTaschen;

namespace ft {
class Mutex;
}

#ifndef CONSTANT_FPS
static const float fps = 0;
#else
static const float fps = CONSTANT_FPS;
#endif

struct arg_struct {
    CompositeFlaschenTaschen* display;
    ft::Mutex* mutex;
    int socket;

    arg_struct(int sock, CompositeFlaschenTaschen* disp, ft::Mutex* mut){
        socket = sock;
        display = disp;
        mutex = mut;
    }
    arg_struct(){}
};

class Server {
public:
    Server();
    virtual ~Server() = 0;

    virtual bool init_server(int port) = 0;
    virtual void run_thread(CompositeFlaschenTaschen *display,
                            ft::Mutex *mutex) = 0;

    static void* periodically_send_to_display(void* ptr);
    static void* receive_data_and_set_display_pixel(void* ptr);

    static void InterruptHandler(int);
    static bool interrupt_received;
    static bool use_constant_async_fps;
};

class TCPServer: public Server {
public:
    TCPServer():Server(){}
    ~TCPServer(){}
    bool init_server(int port);
    void run_thread(CompositeFlaschenTaschen *display,
                    ft::Mutex *mutex);
};

class UDPServer: public Server {
public:
    UDPServer():Server(){}
    ~UDPServer(){}
    bool init_server(int port);
    void run_thread(CompositeFlaschenTaschen *display,
                    ft::Mutex *mutex);
};

// Optional services, currently disabled.
// These should probably be moved out of this project and implemented
// as a bridge.
bool opc_server_init(int port);
void opc_server_run_thread(FlaschenTaschen *display, ft::Mutex *mutex);

bool pixel_pusher_init(const char *interface, FlaschenTaschen *canvas);
void pixel_pusher_run_thread(FlaschenTaschen *display, ft::Mutex *mutex);

#endif // OPC_SERVER_H
