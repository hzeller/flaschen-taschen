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
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <list>

#include "servers.h"

// public interface
static int server_socket = -1;
static struct sockaddr_in addr = {0};
std::list<pthread_t> threads = std::list<pthread_t>();

bool TCPServer::init_server(int port) {
    if ((server_socket = socket(PF_INET6, SOCK_STREAM, 0)) < 0) {
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
    // max 15 sockets
    listen ( server_socket, 15 );
    fprintf(stderr, "TCP-server: ready to listen on %d\n", port);
    return true;
}

void TCPServer::run_thread(CompositeFlaschenTaschen *display,
                             ft::Mutex *mutex) {
    socklen_t addrlen = sizeof (struct sockaddr_in);

    struct sigaction sa = {{0}};  // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53119
    sa.sa_handler = Server::InterruptHandler;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    int new_socket = -1;

    pthread_t thread;
    arg_struct args;
    if (use_constant_async_fps){
        args = arg_struct(0, display, mutex);
        pthread_create(&thread, NULL, Server::periodically_send_to_display, (void*)&args);
        threads.push_back(thread);
    }
    for (;;) {
        if (interrupt_received)
            break;

        new_socket = accept( server_socket, (struct sockaddr *) &addr, &addrlen);

        if (new_socket <= 0){
            continue;
        }

        // since tcp and udp server using the same main loop, the function was outsourced to servers.cc
        // create new thread for every socket connection
        args = arg_struct(new_socket, display, mutex);
        pthread_create(&thread, NULL, Server::receive_data_and_set_display_pixel, (void*)&args);
        threads.push_back(thread);
    }

    while (!threads.empty())
    {
      pthread_join( threads.back(), NULL);
      threads.pop_back();
    }
    close (server_socket);
}
